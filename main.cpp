#include <math.h>
#include <stdio.h>

#include "src/base_types.cpp"
#include "src/dx11.cpp"

void draw_pixel(RenderBuffer *buffer, f32 x, f32 y, u32 color)
{
  buffer->pixel_buffer[(s32)x + (s32)y * buffer->width] = color;
}

void draw_line(RenderBuffer *buffer, f32 x0, f32 y0, f32 x1, f32 y1, u32 color)
{
  f32 dx = x1 - x0;
  f32 dy = y1 - y0;

  f32 fdx = fabsf(dx);
  f32 fdy = fabsf(dy);
  f32 step = fdx >= fdy ? fdx : fdy;

  dx /= step;
  dy /= step;

  f32 x = x0;
  f32 y = y0;

  s32 i = 0;
  while(i <= step)
  {
    draw_pixel(buffer, roundf(x), roundf(y), color);
    x += dx;
    y += dy;
    i++;
  }
}

void present_frame(Pipeline pipeline, RenderBuffer *buffer)
{
  D3D11_MAPPED_SUBRESOURCE mapped_resource;
  pipeline.device_context->Map(buffer->texture_2d, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
  
  u32 *dest = (u32*)mapped_resource.pData;
  u32 *src  = buffer->pixel_buffer;

  memcpy(dest, src, buffer->width * buffer->height * sizeof(u32));
  
  pipeline.device_context->Unmap(buffer->texture_2d, 0);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  // u32 window_width  = 1280;
  // u32 window_height = 720;
  u32 window_width  = 100;
  u32 window_height = 100;
  Window window = create_window("Zoi - A software renderer", window_width, window_height);
  Pipeline pipeline = init_dx11(window);
  RenderBuffer render_buffer = create_main_buffer(pipeline, (u32)pipeline.viewport.Width, (u32)pipeline.viewport.Height);

  for(bool running = true; running;)
  {
    window_message_handler(&running);

    if(os_is_key_pressed(VK_ESCAPE))
    {
      running = false;
    }

    clear_buffer(&render_buffer);

    // draw_pixel(&render_buffer, 10, 10, 0xffff0000);
    // draw_pixel(&render_buffer, 10, 10, 0xffffffff);

    // draw_line(&render_buffer, 30, 30, 70, 80, 0xffffff00);
    // draw_line(&render_buffer, 30, 30, 70, 80, 0xffffffff);
    // draw_line(&render_buffer,  0, 0, 1, 0.5f, 0xffffffff);
    draw_line(&render_buffer, 30, 30, 160, 10, 0xffffffff);
    // draw_line(&render_buffer, 30, 30, 60, 10, 0xffffffff);
    draw_line(&render_buffer, 0, 0, (f32)window_width, (f32)window_height, 0xffffffff);
    draw_line(&render_buffer, 0, 100, (f32)window_width, 100, 0xffffffff);
    
    present_frame(pipeline, &render_buffer);

    render_frame(pipeline, render_buffer);
  }

  return 0;
}