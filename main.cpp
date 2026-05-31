#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "src/base_types.cpp"
#include "src/dx11.cpp"
#include "src/math.cpp"
#include "src/font.cpp"
#include "src/timing.cpp"

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

struct Vertex
{
  V3 position;
  V4 color;
  V2 uv;
  f32 w;
};

enum TextureFilter
{
  NONE,
  BILINEAR,
};

struct Texture
{
  u32 width, height;
  u32 *pixels;
  TextureFilter filter;
  Texture *mipmaps;
  u32 mipmaps_count;
};

struct Mesh
{
  Vertex *vertices;
  u32 vertices_count;
  Texture texture;
};

Texture create_checkboard_texture(u32 width, u32 height)
{
  Texture texture = {};
  texture.width  = width;
  texture.height = height;
  texture.pixels = (u32*)malloc(texture.width * texture.height * sizeof(u32));

  for(u32 y = 0; y < texture.height; y++)
  {
    for(u32 x = 0; x < texture.width; x++)
    {
      // u32 tile_size = 4;
      u32 tile_size = 1;
      if(((x / tile_size) + (y / tile_size)) % 2 == 0)
      {
        texture.pixels[x + y * texture.width] = 0xffffffff;
      }
      else
      {
        texture.pixels[x + y * texture.width] = 0x000000ff;
      }
    }
  }

  return texture;
}

void generate_mipmaps(Texture *texture)
{
  texture->mipmaps_count = (u32)log2f((f32)texture->width);
  texture->mipmaps = (Texture*)malloc(texture->mipmaps_count * sizeof(Texture));

  for(u32 i = 0; i < texture->mipmaps_count; i++)
  {
    Texture *src = (i == 0) ? texture : &texture->mipmaps[i - 1];
    Texture *mipmap = &texture->mipmaps[i];
    mipmap->width  = src->width  / 2;
    mipmap->height = src->height / 2;
    mipmap->pixels = (u32*)malloc(mipmap->width * mipmap->height * sizeof(u32));

    for(u32 y = 0; y < mipmap->height; y++)
    {
      for(u32 x = 0; x < mipmap->width; x++)
      {
        u32 u = x;
        u32 v = y;
        u32 s = x + 1;
        u32 t = y + 1;

        u32 sample_a = src->pixels[u + v * src->width];
        u32 sample_b = src->pixels[s + v * src->width];
        u32 sample_c = src->pixels[u + t * src->width];
        u32 sample_d = src->pixels[s + t * src->width];
        V4 sample0 = v4_from_u32(sample_a);
        V4 sample1 = v4_from_u32(sample_b);
        V4 sample2 = v4_from_u32(sample_c);
        V4 sample3 = v4_from_u32(sample_d);
        
        V4 blend0 = blend(sample0, sample1, 0.5f);
        V4 blend1 = blend(sample2, sample3, 0.5f);
        V4 blend2 = blend(blend0, blend1, 0.5f);

        u32 color = u32_from_v4(blend2);
        mipmap->pixels[x + y * mipmap->width] = color;
      }
    }
  }
}

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

bool is_top_or_left_edge(V3 a, V3 b)
{
  V3 c = b - a;
  // TOP
  if(c.y == 0 && c.x > 0) return true;
  // LEFT
  if(c.y < 0) return true;

  return false;
}

void draw_triangle(RenderBuffer *buffer, Vertex va, Vertex vb, Vertex vc, Texture texture = {})
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
        f32 weight_a = (w0 / area);
        f32 weight_b = (w1 / area);
        f32 weight_c = (w2 / area);

        V4 blend_color = color_a * weight_a + color_b * weight_b + color_c * weight_c;

        if(texture.pixels)
        {
          V2 uva = (va.uv / va.w) * weight_a;
          V2 uvb = (vb.uv / vb.w) * weight_b;
          V2 uvc = (vc.uv / vc.w) * weight_c;
          
          f32 inv_w = (1/va.w) * weight_a + (1/vb.w) * weight_b + (1/vc.w) * weight_c;
          V2 sample_uv = ((uva + uvb + uvc) / inv_w) * V2{(f32)texture.width, (f32)texture.height};
          
          f32 uvx0 = sample_uv.x;
          f32 uvy0 = sample_uv.y;
          f32 uvx1 = sample_uv.x + 1;
          f32 uvy1 = sample_uv.y + 1;

          #if 0
          f32 dx = uvx1 - uvx0;
          f32 dy = uvy1 - uvy0;
          u32 mip_level = (u32)log2f(max(dx, dy));
          texture = texture.mipmaps[mip_level];
          // Recalculate uv
          sample_uv = ((uva + uvb + uvc) / inv_w) * V2{(f32)texture.width, (f32)texture.height};
          #endif

          u32 sample_a = texture.pixels[(u32)uvx0 + (u32)uvy0 * texture.width];
          
          switch(texture.filter)
          {
            case NONE:
            {
              blend_color = v4_from_u32(sample_a);
            }
            break;
            case BILINEAR:
            {
              uvx0 = clamp(uvx0, 0, (f32)texture.width  - 1);
              uvy0 = clamp(uvy0, 0, (f32)texture.height - 1);
              uvx1 = clamp(uvx1, 0, (f32)texture.width  - 1);
              uvy1 = clamp(uvy1, 0, (f32)texture.height - 1);

              f32 blend_factor_x = frac(uvx0);
              f32 blend_factor_y = frac(uvy0);
              
              u32 sample_b = texture.pixels[(u32)uvx1 + (u32)uvy0 * texture.width];
              
              u32 sample_c = texture.pixels[(u32)uvx0 + (u32)uvy1 * texture.width];
              u32 sample_d = texture.pixels[(u32)uvx1 + (u32)uvy1 * texture.width];

              V4 sa = v4_from_u32(sample_a);
              V4 sb = v4_from_u32(sample_b);
              V4 sc = v4_from_u32(sample_c);
              V4 sd = v4_from_u32(sample_d);
              
              V4 blend0 = blend(sa, sb, blend_factor_x);
              V4 blend1 = blend(sc, sd, blend_factor_x);

              V4 blend2 = blend(blend0, blend1, blend_factor_y);
              blend_color = blend2;
            }
            break;
          }
        }

        draw_pixel(buffer, x, y, u32_from_v4(blend_color));
      }
    }
  }
}

union Triangle
{
  struct { V3 a, b, c; };
  V3 vertices[3];
};

Vertex project(Vertex v, Matrix model_to_view, Matrix view_to_projection, u32 window_width, u32 window_height)
{
  Vertex result = {};
  memcpy(&result, &v, sizeof(Vertex));

  V4 projected_position = (V4_from(v.position, 1) * model_to_view * view_to_projection);
  V3 normalized_position = projected_position.rgb / projected_position.w;
  V3 screen_position = ndc_to_screen(normalized_position, window_width, window_height);

  result.position = screen_position;
  result.w = projected_position.w;
  return result;
}

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

void draw_text(RenderBuffer *render_buffer, u32 x, u32 y, char *text)
{
  u32 cursor_x = x;
  while(*text)
  {
    u8 c = *text;
    for(u32 row = 0; row < 8; row++)
    {
      for(u32 col = 0; col < 8; col++)
      {
        s32 should_paint = font[c][row] & (1 << col);
        if(should_paint)
        {
          draw_pixel_alpha(render_buffer, (f32)(cursor_x + col), (f32)(y + row), 0xffffffff, 1);
        }
      }
    }

    text++;
    cursor_x += 8;
  }
}

void draw_mesh(RenderBuffer *render_buffer, Mesh mesh)
{
  u32 total_triangles = mesh.vertices_count / 3;
  for(u32 i = 0; i < total_triangles; i++)
  {
    Vertex va = mesh.vertices[(i * 3) + 0];
    Vertex vb = mesh.vertices[(i * 3) + 1];
    Vertex vc = mesh.vertices[(i * 3) + 2];
    draw_triangle(render_buffer, va, vb, vc, mesh.texture);
  }
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

  Texture checkerboard_texture = create_checkboard_texture(64, 64);
  checkerboard_texture.filter = BILINEAR;
  generate_mipmaps(&checkerboard_texture);

  // Triangle t = {{0,0,0}, {150, 0, 0}, {150, 150, 0}};
  Triangle t = {};
  t.vertices[0] = { 0.93f,  0.0f, 0.0f};
  t.vertices[1] = { 0.63f, -0.2f, 0.3f};
  t.vertices[2] = {-0.33f,  0.2f, 0.3f};

  Triangle top = {};
  top.vertices[0] = {0.9f, -0.8f,  1.0f};
  top.vertices[1] = {0.9f, -0.8f, -1.0f};
  top.vertices[2] = {0.9f,  0.8f,  1.0f};

  Triangle bottom = {};
  bottom.vertices[0] = {0.9f, -0.8f, -1.0f};
  bottom.vertices[1] = {0.9f,  0.8f, -1.0f};
  bottom.vertices[2] = {0.9f,  0.8f,  1.0f};

  V2 uvs[6] =
  {
    {0,1},
    {0,0},
    {1,1},
    {1,1},
    {1,0},
    {0,0},
  };

  Mesh quad = {};
  quad.texture = checkerboard_texture;
  quad.vertices_count = 6;
  quad.vertices = (Vertex*)malloc(quad.vertices_count * sizeof(Vertex));
  for(u32 i = 0; i < quad.vertices_count; i++)
  {
    V3 position = {};
    V2 uv = uvs[i];
    if(i < 3)
    {
      position = top.vertices[i];
    }
    else
    {
      position = bottom.vertices[i-3];
    }

    quad.vertices[i].position = position;
    quad.vertices[i].uv = uv;
    quad.vertices[i].color = {1, 1, 1, 1}; // TODO: Handle both 255 and 1 range.
  }

  Mesh projected_quad = {};
  projected_quad.texture = quad.texture;
  projected_quad.vertices_count = quad.vertices_count;
  projected_quad.vertices = (Vertex*)malloc(quad.vertices_count * sizeof(Vertex));

  // Cube at x:[0.3,0.9] y:[-0.4,0.4] z:[-0.4,0.4]
  V3 v0 = {0.3f, -0.4f, -0.4f};
  V3 v1 = {0.9f, -0.4f, -0.4f};
  V3 v2 = {0.9f,  0.4f, -0.4f};
  V3 v3 = {0.3f,  0.4f, -0.4f};
  V3 v4 = {0.3f, -0.4f,  0.4f};
  V3 v5 = {0.9f, -0.4f,  0.4f};
  V3 v6 = {0.9f,  0.4f,  0.4f};
  V3 v7 = {0.3f,  0.4f,  0.4f};

  // Triangle cube[12] =
  // {
  //   {v0, v4, v7}, {v0, v7, v3}, // near   (x=0.3)
  //   {v1, v2, v6}, {v1, v6, v5}, // far    (x=0.9)
  //   {v0, v1, v5}, {v0, v5, v4}, // bottom (y=-0.4)
  //   {v3, v7, v6}, {v3, v6, v2}, // top    (y=+0.4)
  //   {v0, v3, v2}, {v0, v2, v1}, // back   (z=-0.4)
  //   {v4, v5, v6}, {v4, v6, v7}, // front  (z=+0.4)
  // };

  Triangle cube[12] =
  {
    {v0, v7, v4}, {v0, v3, v7}, // near   (x=0.3)
    {v1, v6, v2}, {v1, v5, v6}, // far    (x=0.9)
    {v0, v5, v1}, {v0, v4, v5}, // bottom (y=-0.4)
    {v3, v6, v7}, {v3, v2, v6}, // top    (y=+0.4)
    {v0, v2, v3}, {v0, v1, v2}, // back   (z=-0.4)
    {v4, v6, v5}, {v4, v7, v6}, // front  (z=+0.4)
  };

  V4 face_colors[6] =
  {
    {1,0,0,1}, // near   - red
    {0,1,0,1}, // far    - green
    {0,0,1,1}, // bottom - blue
    {1,1,0,1}, // top    - yellow
    {1,0,1,1}, // back   - magenta
    {0,1,1,1}, // front  - cyan
  };

  bool hide_triangle = false;
  bool hide_triangle1 = false;

  s64 cpu_frequency = get_os_timer_frequency();
  char fps_text[32];

  // u32 frame_counter = 0;
  // f32 fps = 0;
  // f32 rate_timer = 0;
  // s64 begin_frame_time = 0;

  Time time = init_time();
  Frame frame = {};

  for(bool running = true; running;)
  {
    f32 dt = calculate_dt(&time);
    frame_tick(&frame, dt);

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
      // draw_triangle(&render_buffer, a, b, c);
    }
    if(!hide_triangle1)
    {
      Vertex a = {pbottom.a, {1,0,0,1}};
      Vertex b = {pbottom.b, {0,1,0,1}};
      Vertex c = {pbottom.c, {0,0,1,1}};
      // draw_triangle(&render_buffer, pbottom.a, pbottom.b, pbottom.c, 0x55005555);
      // draw_triangle(&render_buffer, a, b, c);
    }


    for(u32 i = 0; i < quad.vertices_count; i++)
    {
      projected_quad.vertices[i] = project(quad.vertices[i], model_to_view, view_to_projection, render_buffer.width, render_buffer.height);
    }
    draw_mesh(&render_buffer, projected_quad);
    // draw_mesh(&render_buffer, quad);

    // ProfileBlock *cube_profile = begin_profile("Cube", cpu_frequency);
    // {      
    //   for(u32 i = 0; i < 12; i++)
    //   {
    //     Triangle pt = project(cube[i], model_to_view, view_to_projection, render_buffer.width, render_buffer.height);
    //     V4 color = face_colors[i / 2];
    //     Vertex a = {pt.a, color};
    //     Vertex b = {pt.b, color};
    //     Vertex c = {pt.c, color};
    //     draw_triangle(&render_buffer, a, b, c);
    //   }      
    // }
    // end_profile(cube_profile);

    // snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", frame.rate);
    snprintf(fps_text, sizeof(fps_text), "FPS: %.4f", 1/frame.rate);
    // snprintf(fps_text, sizeof(fps_text), "Douglas: %.4f", 1.0);
    // snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", frame.rate_timer);
    draw_text(&render_buffer, 4, 3, fps_text);
    // draw_text(&render_buffer, 4, 3, "FPS");
    
    // snprintf(fps_text, sizeof(fps_text), "%s: %.4f", cube_profile->name, cube_profile->elapsed);
    // draw_text(&render_buffer, 4, 13, fps_text);

    draw_frame(pipeline, &render_buffer);

    present_frame(pipeline, render_buffer);
  }

  return 0;
}