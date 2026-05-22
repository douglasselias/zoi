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

    f32 a0 = (f32)x;
    f32 a1 = fi;
    f32 b0 = (f32)x;
    f32 b1 = fi + 1;

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
        buffer->pixel_buffer[(u32)x + (u32)y * buffer->width] = color;
      }
    }
  }
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

      f32 w0 = cross(cb, pb).z; // Opposite of A
      f32 w1 = cross(ac, pc).z; // Opposite of B
      f32 w2 = cross(ba, pa).z; // Opposite of C

      if(w0 >= 0 && w1 >= 0 && w2 >= 0)
      {
        f32 area = w0 + w1 + w2;

        V4 blend_color = color_a * (w0 / area) + color_b * (w1 / area) + color_c * (w2 / area);
        buffer->pixel_buffer[(u32)x + (u32)y * buffer->width] = u32_from_v4(blend_color);
      }
    }
  }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  u32 window_width  = 2560;
  u32 window_height = 1080;
  // u32 window_width  = 1280;
  // u32 window_height = 720;
  // u32 window_width  = 50;
  // u32 window_height = 50;
  Window window = create_window("Zoi - A software renderer", window_width, window_height);
  Pipeline pipeline = init_gfx(window);
  // RenderBuffer render_buffer = create_main_buffer(pipeline, window_width, window_height);
  RenderBuffer render_buffer = create_main_buffer(pipeline, window_width, window_height);

  for(bool running = true; running;)
  {
    window_message_handler(&running);

    if(os_is_key_pressed(VK_ESCAPE)) // TODO: Windows key is leaking.
    {
      running = false;
    }

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
    draw_line(&render_buffer, 0, 0, (f32)window_width, (f32)window_height, 0xffffff77);
    draw_line_aa(&render_buffer, 0, (f32)window_height, (f32)window_width, 0, 0xffffffff);

    draw_triangle(&render_buffer, {0,0,0}, {50, 0, 0}, {50, 50, 0}, 0x550000ff);

    Vertex va = {{200,   0,   0}, {1,0,0,1}};
    Vertex vb = {{800,   0,   0}, {0,1,0,1}};
    Vertex vc = {{500, 400,   0}, {0,0,1,1}};
    draw_triangle(&render_buffer, va, vb, vc);
    
    present_frame(pipeline, &render_buffer);

    render_frame(pipeline, render_buffer);
  }

  return 0;
}