#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "src/base_types.cpp"
#include "src/dx11.cpp"
#include "src/math.cpp"
#include "src/font.cpp"
#include "src/timing.cpp"
#include "src/string.cpp"
#include "src/obj_parser.cpp"

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
  s32 _ypx2 = ypx1 + 1;

  s32 _x0 = xpx1;
  s32 _y0 = ypx1;
  s32 _x1 = xpx1;
  s32 _y1 = _ypx2;

  if(steep)
  {
    swap(&_x0, &_y0);
    swap(&_x1, &_y1);
  }

  draw_pixel_alpha(buffer, (f32)_x0, (f32)_y0, color, rfpart(yend) * xgap);
  draw_pixel_alpha(buffer, (f32)_x1, (f32)_y1, color,   frac(yend) * xgap);

  // Last endpoint
  f32 xend2 = roundf(x1);
  f32 yend2 = y1 + slope * (xend2 - x1);
  f32 xgap2 = frac(x1 + 0.5f);

  s32 xpx2 = (s32)xend2;
  s32 ypx2 = (s32)floorf(yend2);
  s32 ypx3 = ypx2 + 1;

  s32 __x0 = xpx2;
  s32 __y0 = ypx2;
  s32 __x1 = xpx2;
  s32 __y1 = ypx3;

  if(steep)
  {
    swap(&__x0, &__y0);
    swap(&__x1, &__y1);
  }

  draw_pixel_alpha(buffer, (f32)xpx2, (f32)ypx2, color, rfpart(yend2) * xgap2);
  draw_pixel_alpha(buffer, (f32)xpx2, (f32)ypx3, color,   frac(yend2) * xgap2);

  f32 intery = yend + slope;
  for(s32 x = xpx1 + 1; x < xpx2; x++)
  {
    f32 f = frac(intery);
    f32 fi = floorf(intery);

    f32 a0 = (f32)x, a1 = fi;
    f32 b0 = (f32)x, b1 = fi + 1;

    if(steep)
    {
      swap(&a0, &a1);
      swap(&b0, &b1);
    }
    
    draw_pixel_alpha(buffer, a0, a1, color, 1.0f - f);
    draw_pixel_alpha(buffer, b0, b1, color, f);

    intery += slope;
  }
}

struct Vertex
{
  V3 position;
  V4 color;
  V2 uv;
  f32 w;
  V3 normal;
};

union Triangle
{
  struct { V3 a, b, c; };
  V3 vertices[3];
};

enum TextureFilter : u8
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

void draw_text(RenderBuffer *render_buffer, u32 x, u32 y, char *text)
{
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
          draw_pixel_alpha(render_buffer, (f32)(x + col), (f32)(y + row), 0xffffffff, 1);
        }
      }
    }

    text++;
    x += 8;
  }
}

void draw_text(RenderBuffer *render_buffer, u32 x, u32 y, char *text, u32 scale = 1)
{
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
          for(u32 sy = 0; sy < scale; sy++)
          {
            for(u32 sx = 0; sx < scale; sx++)
            {
              draw_pixel_alpha(render_buffer, (f32)(x + col * scale + sx), (f32)(y + row * scale + sy), 0xffffffff, 1);
            }
          }
        }
      }
    }

    text++;
    x += 8 * scale;
  }
}

bool is_top_or_left_edge(V3 a, V3 b)
{
  V3 c = b - a;
  bool is_top  = c.y == 0 && c.x > 0;
  bool is_left = c.y < 0;
  return is_top || is_left;
}

V3 calculate_barycentric(V3 p, V3 a, V3 b, V3 c)
{
  V3 result = {};
  result.x = cross(p-b, c-b).z; // Opposite of A
  result.y = cross(p-c, a-c).z; // Opposite of B
  result.z = cross(p-a, b-a).z; // Opposite of C
  return result;
}

bool is_inside_triangle(V3 a, V3 b, V3 c, V3 bary)
{
  bool is_cb_top_left = is_top_or_left_edge(c, b);
  bool is_ac_top_left = is_top_or_left_edge(a, c);
  bool is_ba_top_left = is_top_or_left_edge(b, a);

  bool passed_edge_a = bary.x < 0 || (bary.x == 0 && is_cb_top_left);
  bool passed_edge_b = bary.y < 0 || (bary.y == 0 && is_ac_top_left);
  bool passed_edge_c = bary.z < 0 || (bary.z == 0 && is_ba_top_left);

  return passed_edge_a && passed_edge_b && passed_edge_c;
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

      V3 p0 = {x + 0.25f, y + 0.25f, 0};
      V3 p1 = {x + 0.75f, y + 0.25f, 0};
      V3 p2 = {x + 0.25f, y + 0.75f, 0};
      V3 p3 = {x + 0.75f, y + 0.75f, 0};

      V3 bary0 = calculate_barycentric(p0, a, b, c);
      V3 bary1 = calculate_barycentric(p1, a, b, c);
      V3 bary2 = calculate_barycentric(p2, a, b, c);
      V3 bary3 = calculate_barycentric(p3, a, b, c);

      f32 coverage = 0;
      if(is_inside_triangle(a, b, c, bary0)) coverage += 0.25f;
      if(is_inside_triangle(a, b, c, bary1)) coverage += 0.25f;
      if(is_inside_triangle(a, b, c, bary2)) coverage += 0.25f;
      if(is_inside_triangle(a, b, c, bary3)) coverage += 0.25f;

      V3 bary = calculate_barycentric(p, a, b, c);
      bool inside = is_inside_triangle(a, b, c, bary);

      // Hollow triangle
      #if 0
      f32 area = bary.x + bary.y + bary.z;
      bool passed_edge_a = (bary.x / area) < 0.05f || (bary.x == 0 && is_cb_top_left);
      bool passed_edge_b = (bary.y / area) < 0.05f || (bary.y == 0 && is_ac_top_left);
      bool passed_edge_c = (bary.z / area) < 0.05f || (bary.z == 0 && is_ba_top_left);
      #endif
 
      // Show overdraw
      #if 0
      bool passed_edge_a = bary.x <= 0;
      bool passed_edge_b = bary.y <= 0;
      bool passed_edge_c = bary.z <= 0;
      #endif

      if(coverage > 0)
      {
        f32 area = bary.x + bary.y + bary.z;
        f32 weight_a = (bary.x / area);
        f32 weight_b = (bary.y / area);
        f32 weight_c = (bary.z / area);

        V4 blend_color = color_a * weight_a + color_b * weight_b + color_c * weight_c;

        f32 inv_w = (1/va.w) * weight_a + (1/vb.w) * weight_b + (1/vc.w) * weight_c;

        if(texture.pixels)
        {
          V2 uva = (va.uv / va.w) * weight_a;
          V2 uvb = (vb.uv / vb.w) * weight_b;
          V2 uvc = (vc.uv / vc.w) * weight_c;
          
          V2 sample_uv = ((uva + uvb + uvc) / inv_w) * V2{(f32)texture.width, (f32)texture.height};
          
          f32 uvx0 = sample_uv.x;
          f32 uvy0 = sample_uv.y;
          f32 uvx1 = sample_uv.x + 1;
          f32 uvy1 = sample_uv.y + 1;
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

        if(0 <= x && x < buffer->width && 0 <= y && y < buffer->height)
        {
          f32 *current_depth = &buffer->z_buffer[(u32)x + (u32)y * buffer->width];

          if(current_depth != null && inv_w > *current_depth)
          {
            *current_depth = inv_w;
            blend_color = blend_color;

            for(u8 i = 0; i < 3; i++)
            {
              blend_color[i] = (blend_color[i]);
            }

            V3 gamma = pow(blend_color.rgb, 1.0f/2.2f);
            draw_pixel_alpha(buffer, x, y, u32_from_v4(V4_from(gamma, blend_color.w)), coverage);
          }
        }
      }
    }
  }
}

bool is_outside_frustum(V3 p)
{
  bool outside_of_x = p.x > 1 || p.x < -1;
  bool outside_of_y = p.y > 1 || p.y < -1;
  bool outside_of_z = p.z > 1 || p.z < -1;
  return outside_of_x || outside_of_y || outside_of_z;
}

void draw_mesh(RenderBuffer *render_buffer, Mesh mesh, Matrix model_to_view, Matrix view_to_projection, u32 render_width, u32 render_height)
{
  u32 total_triangles = mesh.vertices_count / 3;

  Vertex vs[3] = {};
  bool outside[3] = {};
  V3 ndc[3] = {};
  for(u32 i = 0; i < total_triangles; i++)
  {
    V3 a = mesh.vertices[(i * 3) + 0].position;
    V3 b = mesh.vertices[(i * 3) + 1].position;
    V3 c = mesh.vertices[(i * 3) + 2].position;

    for(u32 j = 0; j < 3; j++)
    {
      Vertex mv = mesh.vertices[(i * 3) + j];

      Vertex result = {};
      memcpy(&result, &mv, sizeof(Vertex));

      V4 projected_position = (V4_from(result.position, 1) * model_to_view * view_to_projection);
      V3 normalized_position = projected_position.rgb / projected_position.w;
      ndc[j] = normalized_position;
      V3 screen_position = ndc_to_screen(normalized_position, render_width, render_height);

      result.position = screen_position;
      result.w = projected_position.w;

      vs[j] = result;

      outside[j] = is_outside_frustum(ndc[j]);
    }

    if(outside[0] && outside[1] && outside[2])
    {
      continue;
    }

    draw_triangle(render_buffer, vs[0], vs[1], vs[2], mesh.texture);
  }
}

Mesh mesh_from_obj(Obj obj)
{
  Mesh mesh = {};
  mesh.vertices_count = obj.faces_count * 6;
  mesh.vertices = (Vertex*)calloc(mesh.vertices_count, sizeof(Vertex));

  u32 vi = 0;
  u32 tri[6] = {2, 1, 0, 3, 2, 0};
  for(u32 i = 0; i < obj.faces_count; i++)
  {
    Face face = obj.faces[i];
    for(u32 j = 0; j < 6; j++)
    {
      u32 k = tri[j];
      mesh.vertices[vi].position = obj.vertices[face.indexes[k].v - 1];
      mesh.vertices[vi].uv       = obj.uvs     [face.indexes[k].u - 1];
      mesh.vertices[vi].normal   = obj.normals [face.indexes[k].n - 1];
      vi++;
    }
  }

  return mesh;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  u32 window_width  = 2560;
  u32 window_height = 1080;
  window_width  = 1920;
  window_height = 1080;
  window_width  = 1280;
  window_height = 720;
  #if 0
  window_width  = 1024;
  window_height = 768;
  window_width  = 800;
  window_height = 600;
  window_width  = 640;
  window_height = 480;
  window_width  = 320;
  window_height = 240;
  window_width  = 50;
  window_height = 50;
  #endif

  test_slice();
  test_split();

  Window window = create_window("Zoi - A software renderer", window_width, window_height);
  Pipeline pipeline = init_gfx(window);
  u32 render_scale = 1;
  RenderBuffer render_buffer = create_main_buffer(pipeline, window_width / render_scale, window_height / render_scale);

  CameraActions camera_actions = {};
  Camera camera = {};
  camera.position = {-1.0f, 0, 0};
  f32 fov_y  = 0.25f / 2.0f;
  f32 z_near = 0.01f;
  f32 aspect_ratio = (f32)window_width / (f32)window_height;

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

  V3 v0 = {0.3f, -0.4f, -0.4f};
  V3 v1 = {0.9f, -0.4f, -0.4f};
  V3 v2 = {0.9f,  0.4f, -0.4f};
  V3 v3 = {0.3f,  0.4f, -0.4f};
  V3 v4 = {0.3f, -0.4f,  0.4f};
  V3 v5 = {0.9f, -0.4f,  0.4f};
  V3 v6 = {0.9f,  0.4f,  0.4f};
  V3 v7 = {0.3f,  0.4f,  0.4f};

  V3 cube[] =
  {
    v0, v7, v4, v0, v3, v7, // near
    v1, v6, v2, v1, v5, v6, // far
    v0, v5, v1, v0, v4, v5, // bottom
    v3, v6, v7, v3, v2, v6, // top
    v0, v2, v3, v0, v1, v2, // back
    v4, v6, v5, v4, v7, v6, // front
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

  Mesh cube_mesh = {};
  cube_mesh.vertices_count = 12 * 3;
  cube_mesh.vertices = (Vertex*)malloc(cube_mesh.vertices_count * sizeof(Vertex));
  u32 face_color_index = 0;
  for(u32 i = 0; i < cube_mesh.vertices_count; i++)
  {
    cube_mesh.vertices[i].position = cube[i];
    cube_mesh.vertices[i].color = face_colors[i / 6];
  }

  s64 cpu_frequency = get_os_timer_frequency();
  char fps_text[32];

  Time time = init_time();
  Frame frame = {};

  Obj cube_obj = parse_obj("assets/cube.obj");
  Mesh cube_obj_mesh = mesh_from_obj(cube_obj);
  f32 rot_angle = 0;

  face_color_index = 0;
  for(u32 i = 0; i < cube_obj_mesh.vertices_count; i++)
  {
    cube_obj_mesh.vertices[i].color = face_colors[i/6];
  }

  for(bool running = true; running;)
  {
    f32 dt = calculate_dt(&time);
    frame_tick(&frame, dt);

    WindowMessageType window_message_type;
    while(poll_window_message(&window_message_type))
    {
      switch(window_message_type)
      {
        case MSG_QUIT: running = false; break;
        case MSG_NONE: break;
      }
    }

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

    update_camera(camera_actions, &camera, dt);

    Matrix model_to_view = create_view_matrix(camera.position, camera.target);
    Matrix view_to_projection = create_perspective_projection_matrix(aspect_ratio, fov_y, z_near);

    Matrix view_to_orthograpic = create_orthographic_projection_matrix(window_width / 2.0f, window_height / 2.0f, 0.01f, 100.0f);

    clear_buffer(&render_buffer);

    #if 0
    u32 pixel_size = 32;
    for(f32 y = 0; y < pixel_size; y++)
    {
      for(f32 x = 0; x < pixel_size; x++)
      {
        draw_pixel(&render_buffer, x + (0 * pixel_size), y, 0xff0000ff);
        draw_pixel(&render_buffer, x + (1 * pixel_size), y, 0x00ff00ff);
        draw_pixel(&render_buffer, x + (2 * pixel_size), y, 0x0000ffff);
        draw_pixel(&render_buffer, x + (3 * pixel_size), y, 0xffff00ff);
      }
    }
    #endif

    draw_mesh(&render_buffer, quad, model_to_view, view_to_projection, render_buffer.width, render_buffer.height);
    // draw_mesh(&render_buffer, cube_mesh, model_to_view, view_to_projection, render_buffer.width, render_buffer.height, sun);
    Matrix scale = create_scale_matrix(0.01f);
    Matrix position = create_translation_matrix(200, 0, 0);
    Matrix rotation_x = create_x_axis_rotation_matrix(rot_angle + 0.001f);
    Matrix rotation_y = create_y_axis_rotation_matrix(rot_angle + 0.005f);
    Matrix rotation_z = create_z_axis_rotation_matrix(rot_angle + 0.009f);
    Matrix rotation = rotation_x * rotation_y * rotation_z;
    rot_angle += 0.1f * dt;
    ProfileBlock *cube_profile = begin_profile("Cube", cpu_frequency);
    draw_mesh(&render_buffer, cube_obj_mesh, rotation * position * scale * model_to_view, view_to_projection, render_buffer.width, render_buffer.height);
    end_profile(cube_profile);

    u32 font_scale = 4;
    u32 text_y = 3;

    snprintf(fps_text, sizeof(fps_text), "%.4fms", 1/frame.rate);
    draw_text(&render_buffer, 4, text_y, fps_text, font_scale);

    text_y += font_scale * 10;
    snprintf(fps_text, sizeof(fps_text), "%.2f FPS", frame.rate);
    draw_text(&render_buffer, 4, text_y, fps_text, font_scale);

    text_y += font_scale * 10;
    snprintf(fps_text, sizeof(fps_text), "%s: %.4f", cube_profile->name, cube_profile->elapsed);
    draw_text(&render_buffer, 4, text_y, fps_text, font_scale);

    draw_frame(pipeline, &render_buffer);

    present_frame(pipeline, render_buffer);
  }

  return 0;
}