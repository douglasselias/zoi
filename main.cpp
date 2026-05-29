#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "src/base_types.cpp"
#include "src/dx11.cpp"
#include "src/math.cpp"

void draw_pixel(RenderBuffer *buffer, f32 x, f32 y, u32 color)
{
  u32 index = (u32)x + (u32)y * buffer->width;
  if(index < buffer->width * buffer->height) // TODO: Maybe use a buffer->area
  {
    buffer->pixel_buffer[index] = color;
  }
}

void draw_pixel_alpha(RenderBuffer *buffer, f32 x, f32 y, u32 color, f32 alpha)
{
  u32 index = (u32)x + (u32)y * buffer->width;
  if(index >= buffer->width * buffer->height) return;

  u8 src_r = (color >> 24) & 0xff;
  u8 src_g = (color >> 16) & 0xff;
  u8 src_b = (color >>  8) & 0xff;

  u32 dest = buffer->pixel_buffer[index];
  u8 dest_r = (dest >> 24) & 0xff;
  u8 dest_g = (dest >> 16) & 0xff;
  u8 dest_b = (dest >>  8) & 0xff;

  u8 r = (u8)(src_r * alpha + dest_r * (1.0f - alpha));
  u8 g = (u8)(src_g * alpha + dest_g * (1.0f - alpha));
  u8 b = (u8)(src_b * alpha + dest_b * (1.0f - alpha));

  buffer->pixel_buffer[index] = (r << 24) | (g << 16) | (b << 8) | 0xff;
}

void draw_line(RenderBuffer *buffer, f32 x0, f32 y0, f32 x1, f32 y1, u32 color)
{
  f32 dx = x1 - x0;
  f32 dy = y1 - y0;

  f32 fdx = fabsf(dx);
  f32 fdy = fabsf(dy);
  f32 steps = fdx >= fdy ? fdx : fdy;

  dx /= steps;
  dy /= steps;

  f32 x = x0;
  f32 y = y0;

  s32 i = 0;
  while(i <= steps)
  {
    draw_pixel(buffer, roundf(x), roundf(y), color);
    x += dx;
    y += dy;
    i++;
  }
}

f32 frac(f32 x) { return x - floorf(x); }
f32 rfpart(f32 x) { return 1.0f - frac(x); }

void swap(f32 *a, f32 *b)
{
  f32 c = *a;
  *a = *b;
  *b = c;
}

void draw_line_aa(RenderBuffer *buffer, f32 x0, f32 y0, f32 x1, f32 y1, u32 color)
{
  f32 dy = y1 - y0;
  f32 dx = x1 - x0;
  bool steep = fabsf(dy) > fabsf(dx);

  if(steep)
  {
    swap(&x0, &y0);
    swap(&x1, &y1);
  }

  if(x0 > x1)
  {
    swap(&x0, &x1);
    swap(&y0, &y1);
  }

  f32 slope = dy / dx;

  f32 xend = roundf(x0);
  f32 yend = y0 + slope * (xend - x0);
  f32 xgap = rfpart(x0 + 0.5f);

  s32 xpx1 = (s32)xend;
  s32 ypx1 = (s32)floorf(yend);

  if(steep)
  {
    draw_pixel_alpha(buffer, (f32)ypx1,     (f32)xpx1, color, rfpart(yend) * xgap);
    draw_pixel_alpha(buffer, (f32)ypx1 + 1, (f32)xpx1, color,   frac(yend) * xgap);
  }
  else
  {
    draw_pixel_alpha(buffer, (f32)xpx1, (f32)ypx1,     color, rfpart(yend) * xgap);
    draw_pixel_alpha(buffer, (f32)xpx1, (f32)ypx1 + 1, color,   frac(yend) * xgap);
  }

  ///////////////

  // Last endpoint
  f32 xend2 = roundf(x1);
  f32 yend2 = y1 + slope * (xend2 - x1);
  f32 xgap2 = frac(x1 + 0.5f);

  s32 xpx2 = (s32)xend2;
  s32 ypx2 = (s32)floorf(yend2);

  if(steep)
  {
    draw_pixel_alpha(buffer, (f32)ypx2,     (f32)xpx2, color, rfpart(yend2) * xgap2);
    draw_pixel_alpha(buffer, (f32)ypx2 + 1, (f32)xpx2, color,   frac(yend2) * xgap2);
  }
  else
  {
    draw_pixel_alpha(buffer, (f32)xpx2, (f32)ypx2,     color, rfpart(yend2) * xgap2);
    draw_pixel_alpha(buffer, (f32)xpx2, (f32)ypx2 + 1, color,   frac(yend2) * xgap2);
  }

  f32 intery = yend + slope;
  for(s32 x = xpx1 + 1; x < xpx2; x++)
  {
    f32 f = frac(intery);
    f32 fi = floorf(intery);

    f32 a0 = (f32)x, a1 = fi;
    f32 b0 = (f32)x, b1 = fi + 1;

    if(steep)
    {
      // draw_pixel_alpha(buffer, floorf(intery),     (f32)x, color, 1.0f - f);
      // draw_pixel_alpha(buffer, floorf(intery) + 1, (f32)x, color, f);
      swap(&a0, &a1);
      swap(&b0, &b1);
    }
    // else
    {
      // draw_pixel_alpha(buffer, (f32)x, floorf(intery),     color, 1.0f - f);
      // draw_pixel_alpha(buffer, (f32)x, floorf(intery) + 1, color, f);
      draw_pixel_alpha(buffer, a0, a1, color, 1.0f - f);
      draw_pixel_alpha(buffer, b0, b1, color, f);
    }

    intery += slope;
  }
}

bool inside_triangle(V3 p, V3 a, V3 b, V3 c)
{
  V3 pa = p - a;
  V3 pb = p - b;
  V3 pc = p - c;

  V3 ba = b - a;
  V3 cb = c - b;
  V3 ac = a - c;

  f32 w0 = cross(ba, pa).z;
  f32 w1 = cross(cb, pb).z;
  f32 w2 = cross(ac, pc).z;

  if(w0 >= 0 && w1 >= 0 && w2 >= 0)
  {
    return true;
  }

  return false;
}

// bool inside_triangle(V3 p, V3 a, V3 b, V3 c)
// {
//   V3 weights = {};
//   weights.x = cross(b-a, p-a).z;
//   weights.y = cross(c-b, p-b).z;
//   weights.z = cross(a-c, p-c).z;

//   if(weights.x >= 0 && weights.y >= 0 && weights.z >= 0)
//   {
//     return true;
//   }

//   return false;
// }

struct Vertex
{
  V3 position;
  V4 color;
};

void draw_triangle(RenderBuffer *buffer, V3 a, V3 b, V3 c, u32 color)
{
  f32 min_x = min(min(a.x, b.x), c.x);
  f32 max_x = max(max(a.x, b.x), c.x);
  f32 min_y = min(min(a.y, b.y), c.y);
  f32 max_y = max(max(a.y, b.y), c.y);

  for(f32 y = min_y; y < max_y; y++)
  {
    for(f32 x = min_x; x < max_x; x++)
    {
      V3 p = {x, y, 0};

      V3 pa = p - a;
      V3 pb = p - b;
      V3 pc = p - c;

      V3 ba = b - a;
      V3 cb = c - b;
      V3 ac = a - c;

      f32 w0 = cross(ba, pa).z;
      f32 w1 = cross(cb, pb).z;
      f32 w2 = cross(ac, pc).z;

      if(w0 >= 0 && w1 >= 0 && w2 >= 0)
      {
        draw_pixel(buffer, x, y, color);
        // buffer->pixel_buffer[(u32)x + (u32)y * buffer->width] = color;
      }
    }
  }
}

// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules
// A top edge, is an edge that is exactly horizontal and is above the other edges.
// A bottom edge, is an edge that is exactly horizontal and is below the other edges.
// A left edge, is an edge that is not exactly horizontal and is on the left side of the triangle. A triangle can have one or two left edges.
// A right edge, is an edge that is not exactly horizontal and is on the right side of the triangle. A triangle can have one or two right edges.

bool is_top_or_left_edge(V3 a, V3 b)
{
  V3 c = b - a;
  // TOP
  if(c.y == 0 && c.x > 0) return true;
  // LEFT
  if(c.y < 0) return true;

  return false;
}

void draw_triangle(RenderBuffer *buffer, Vertex va, Vertex vb, Vertex vc)
{
  V3 a = va.position;
  V3 b = vb.position;
  V3 c = vc.position;
  V4 color_a = va.color;
  V4 color_b = vb.color;
  V4 color_c = vc.color;

  f32 min_x = min(min(a.x, b.x), c.x);
  f32 max_x = max(max(a.x, b.x), c.x);
  f32 min_y = min(min(a.y, b.y), c.y);
  f32 max_y = max(max(a.y, b.y), c.y);

  for(f32 y = min_y; y < max_y; y++)
  {
    for(f32 x = min_x; x < max_x; x++)
    {
      V3 p = {x, y, 0};

      V3 pa = p - a;
      V3 pb = p - b;
      V3 pc = p - c;

      V3 ba = b - a;
      V3 cb = c - b;
      V3 ac = a - c;

      f32 w0 = cross(pb, cb).z; // Opposite of A
      f32 w1 = cross(pc, ac).z; // Opposite of B
      f32 w2 = cross(pa, ba).z; // Opposite of C
      
      bool is_cb_top_left = is_top_or_left_edge(c, b);
      bool is_ac_top_left = is_top_or_left_edge(a, c);
      bool is_ba_top_left = is_top_or_left_edge(b, a);

      bool passed_edge_a = w0 < 0 || (w0 == 0 && is_cb_top_left);
      bool passed_edge_b = w1 < 0 || (w1 == 0 && is_ac_top_left);
      bool passed_edge_c = w2 < 0 || (w2 == 0 && is_ba_top_left);

      // TODO: Hollow triangle
      #if 0
      f32 area = w0 + w1 + w2;
      bool passed_edge_a = (w0 / area) < 0.05f || (w0 == 0 && is_cb_top_left);
      bool passed_edge_b = (w1 / area) < 0.05f || (w1 == 0 && is_ac_top_left);
      bool passed_edge_c = (w2 / area) < 0.05f || (w2 == 0 && is_ba_top_left);
      #endif
 
      // TODO: Show overdraw
      #if 0
      bool passed_edge_a = w0 <= 0;
      bool passed_edge_b = w1 <= 0;
      bool passed_edge_c = w2 <= 0;
      #endif

      if(passed_edge_a && passed_edge_b && passed_edge_c)
      {
        f32 area = w0 + w1 + w2;
        V4 blend_color = color_a * (w0 / area) + color_b * (w1 / area) + color_c * (w2 / area);
        draw_pixel(buffer, x, y, u32_from_v4(blend_color));
        // buffer->pixel_buffer[(u32)x + (u32)y * buffer->width] = u32_from_v4(blend_color);
      }
      // else draw_pixel(buffer, x, y, (0x000000ff));
    }
  }
}

struct Triangle { V3 a, b, c; };

Triangle project(Triangle t, Matrix model_to_view, Matrix view_to_projection, u32 window_width, u32 window_height)
{
  V4 cta = (V4_from(t.a, 1) * model_to_view * view_to_projection);
  V4 ctb = (V4_from(t.b, 1) * model_to_view * view_to_projection);
  V4 ctc = (V4_from(t.c, 1) * model_to_view * view_to_projection);
  V3 ndc_ta = cta.rgb / cta.w;
  V3 ndc_tb = ctb.rgb / ctb.w;
  V3 ndc_tc = ctc.rgb / ctc.w;
  V3 ta = ndc_to_screen(ndc_ta, window_width, window_height);
  V3 tb = ndc_to_screen(ndc_tb, window_width, window_height);
  V3 tc = ndc_to_screen(ndc_tc, window_width, window_height);
  return {ta, tb, tc};
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  // u32 window_width  = 2560;
  // u32 window_height = 1080;
  // u32 window_width  = 1280;
  // u32 window_height = 720;
  // u32 window_width  = 1024;
  // u32 window_height = 768;
  // u32 window_width  = 800;
  // u32 window_height = 600;
  u32 window_width  = 640;
  u32 window_height = 480;
  // u32 window_width  = 50;
  // u32 window_height = 50;
  Window window = create_window("Zoi - A software renderer", window_width, window_height);
  Pipeline pipeline = init_gfx(window);
  u32 render_scale = 4;
  RenderBuffer render_buffer = create_main_buffer(pipeline, window_width / render_scale, window_height / render_scale);

  CameraActions camera_actions = {};
  Camera camera = {};
  camera.position = {-1.0f, 0, 0};
  f32 fov_y  = 0.25f / 2.0f;
  f32 z_near = 0.01f;
  f32 aspect_ratio = (f32)window_width / (f32)window_height;

  // Triangle t = {{0,0,0}, {150, 0, 0}, {150, 150, 0}};
  Triangle t =
  {
    {0.93f,  0.0f, 0.0f},
    {0.63f, -0.2f, 0.3f},
    {-0.33f,  0.2f, 0.3f},
  };

  Triangle top =
  {
    {0.9f, -0.8f,  1.0f},
    {0.9f, -0.8f, -1.0f},
    {0.9f,  0.8f,  1.0f},
  };

  Triangle bottom =
  {
    {0.9f, -0.8f, -1.0f},
    {0.9f,  0.8f, -1.0f},
    {0.9f,  0.8f,  1.0f},
  };

  bool hide_triangle = false;
  bool hide_triangle1 = false;

  for(bool running = true; running;)
  {
    WindowMessageType window_message_type;
    while(poll_window_message(&window_message_type))
    {
      switch(window_message_type)
      {
        case MSG_NONE: break;
        case MSG_QUIT:
          running = false;
        break;
      }
    }

    if(is_key_pressed(KEY_G)) hide_triangle = !hide_triangle;
    if(is_key_pressed(KEY_H)) hide_triangle1 = !hide_triangle1;

    if(is_key_pressed(KEY_ESCAPE))
    {
      running = false;
    }

    camera_actions.is_pressing_forward  = is_key_down(KEY_W);
    camera_actions.is_pressing_backward = is_key_down(KEY_S);
    camera_actions.is_pressing_left     = is_key_down(KEY_A);
    camera_actions.is_pressing_right    = is_key_down(KEY_D);
    camera_actions.is_pressing_up       = is_key_down(KEY_Q);
    camera_actions.is_pressing_down     = is_key_down(KEY_E);

    camera_actions.is_looking_left  = is_key_down(KEY_J);
    camera_actions.is_looking_right = is_key_down(KEY_L);
    camera_actions.is_looking_up    = is_key_down(KEY_I);
    camera_actions.is_looking_down  = is_key_down(KEY_K);

    f32 FIXED_DT = 1.0f / 60.0f;
    update_camera(camera_actions, &camera, FIXED_DT);

    Matrix model_to_view = create_view_matrix(camera.position, camera.target);
    Matrix view_to_projection = create_perspective_projection_matrix(aspect_ratio, fov_y, z_near);

    clear_buffer(&render_buffer);

    for(u32 x = 0; x < render_buffer.width; x++)
    {
      // draw_pixel(&render_buffer, (f32)x, 50, 0xffff0000);
      // draw_pixel(&render_buffer, (f32)x, 50, 0xff0000ff);
    }
    // draw_pixel(&render_buffer, 10, 10, 0xfff00fff);
    // draw_pixel(&render_buffer, 10, 10, 0x0000ff00);
    u32 pixel_size = 6;
    for(f32 y = 0; y < pixel_size; y++)
    {
      for(f32 x = 0; x < pixel_size; x++)
      {
        // draw_pixel(&render_buffer, x + (0 * pixel_size), y, 0xff000000);
        // draw_pixel(&render_buffer, x + (1 * pixel_size), y, 0x00ff0000);
        // draw_pixel(&render_buffer, x + (2 * pixel_size), y, 0x0000ff00);
      }
    }

    // draw_line(&render_buffer,  0, 0, 1, 0.5f, 0xffffffff);
    // draw_line(&render_buffer, 30, 30, 60, 10, 0xffffffff);
    // draw_line(&render_buffer, 0, 0, (f32)window_width, (f32)window_height, 0xffffffff);
    // draw_line(&render_buffer, 0, 0, (f32)window_width, (f32)window_height, 0xffffff77);
    // draw_line_aa(&render_buffer, 0, (f32)window_height, (f32)window_width, 0, 0xffffffff);

    // draw_triangle(&render_buffer, {0,0,0}, {50, 0, 0}, {50, 50, 0}, 0x550000ff);
    V4 cta = (V4_from(t.a, 1) * model_to_view * view_to_projection);
    V4 ctb = (V4_from(t.b, 1) * model_to_view * view_to_projection);
    V4 ctc = (V4_from(t.c, 1) * model_to_view * view_to_projection);
    V3 ndc_ta = cta.rgb / cta.w;
    V3 ndc_tb = ctb.rgb / ctb.w;
    V3 ndc_tc = ctc.rgb / ctc.w;
    V3 ta = ndc_to_screen(ndc_ta, render_buffer.width, render_buffer.height);
    V3 tb = ndc_to_screen(ndc_tb, render_buffer.width, render_buffer.height);
    V3 tc = ndc_to_screen(ndc_tc, render_buffer.width, render_buffer.height);

    // draw_triangle(&render_buffer, t.a, t.b, t.c, 0x550000ff);
    // draw_triangle(&render_buffer, ta, tb, tc, 0x550000ff);

    Triangle ptop    = project(top, model_to_view, view_to_projection, render_buffer.width, render_buffer.height);
    Triangle pbottom = project(bottom, model_to_view, view_to_projection, render_buffer.width, render_buffer.height);
    if(!hide_triangle)
    {
      Vertex a = {ptop.a, {1,0,0,1}};
      Vertex b = {ptop.b, {0,1,0,1}};
      Vertex c = {ptop.c, {0,0,1,1}};
      // draw_triangle(&render_buffer, ptop.a, ptop.b, ptop.c, 0x555500ff);
      draw_triangle(&render_buffer, a, b, c);
    }
    if(!hide_triangle1)
    {
      Vertex a = {pbottom.a, {1,0,0,1}};
      Vertex b = {pbottom.b, {0,1,0,1}};
      Vertex c = {pbottom.c, {0,0,1,1}};
      // draw_triangle(&render_buffer, pbottom.a, pbottom.b, pbottom.c, 0x55005555);
      draw_triangle(&render_buffer, a, b, c);
    }

    draw_frame(pipeline, &render_buffer);

    present_frame(pipeline, render_buffer);
  }

  return 0;
}