#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <string.h>
#include <stdlib.h>

s64 get_os_timer()
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result.QuadPart;
}

s64 get_os_timer_frequency()
{
  LARGE_INTEGER result;
  QueryPerformanceFrequency(&result);
  return result.QuadPart;
}

struct ProfileBlock
{
  char *name;
  s64 frequency;
  s64 begin_time;
  f64 elapsed;
};

ProfileBlock profile_blocks[100] = {};
u32 profile_block_index = 0;

ProfileBlock* begin_profile(char *name, s64 frequency)
{
  u32 i = profile_block_index;
  profile_block_index = (profile_block_index + 1) % 100;
  ProfileBlock *block = &profile_blocks[i];
  block->name = name;
  block->begin_time = get_os_timer();
  block->frequency = frequency;
  return block;
}

void end_profile(ProfileBlock *block)
{
  // block->elapsed = (get_os_timer() - block->begin_time) / (f64)block->frequency * 1000.0;
  block->elapsed = (get_os_timer() - block->begin_time) / (f64)block->frequency;
}

enum Key : u8
{
  KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
  KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
  KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
  KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
  KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
  KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
  KEY_SPACE, KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB,
  KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
  KEY_SHIFT, KEY_CTRL, KEY_ALT,
  KEY_F1, KEY_F2, KEY_F3, KEY_F4,  KEY_F5,  KEY_F6,
  KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
};

s32 internal_convert_key_to_os_key(Key key)
{
  switch(key)
  {
    case KEY_A: return 'A';
    case KEY_B: return 'B';
    case KEY_C: return 'C';
    case KEY_D: return 'D';
    case KEY_E: return 'E';
    case KEY_F: return 'F';
    case KEY_G: return 'G';
    case KEY_H: return 'H';
    case KEY_I: return 'I';
    case KEY_J: return 'J';
    case KEY_K: return 'K';
    case KEY_L: return 'L';
    case KEY_M: return 'M';
    case KEY_N: return 'N';
    case KEY_O: return 'O';
    case KEY_P: return 'P';
    case KEY_Q: return 'Q';
    case KEY_R: return 'R';
    case KEY_S: return 'S';
    case KEY_T: return 'T';
    case KEY_U: return 'U';
    case KEY_V: return 'V';
    case KEY_W: return 'W';
    case KEY_X: return 'X';
    case KEY_Y: return 'Y';
    case KEY_Z: return 'Z';
    case KEY_0: return '0';
    case KEY_1: return '1';
    case KEY_2: return '2';
    case KEY_3: return '3';
    case KEY_4: return '4';
    case KEY_5: return '5';
    case KEY_6: return '6';
    case KEY_7: return '7';
    case KEY_8: return '8';
    case KEY_9: return '9';
    case KEY_SPACE:     return VK_SPACE;
    case KEY_ENTER:     return VK_RETURN;
    case KEY_ESCAPE:    return VK_ESCAPE;
    case KEY_BACKSPACE: return VK_BACK;
    case KEY_TAB:       return VK_TAB;
    case KEY_LEFT:      return VK_LEFT;
    case KEY_RIGHT:     return VK_RIGHT;
    case KEY_UP:        return VK_UP;
    case KEY_DOWN:      return VK_DOWN;
    case KEY_SHIFT:     return VK_SHIFT;
    case KEY_CTRL:      return VK_CONTROL;
    case KEY_ALT:       return VK_MENU;
    case KEY_F1:        return VK_F1;
    case KEY_F2:        return VK_F2;
    case KEY_F3:        return VK_F3;
    case KEY_F4:        return VK_F4;
    case KEY_F5:        return VK_F5;
    case KEY_F6:        return VK_F6;
    case KEY_F7:        return VK_F7;
    case KEY_F8:        return VK_F8;
    case KEY_F9:        return VK_F9;
    case KEY_F10:       return VK_F10;
    case KEY_F11:       return VK_F11;
    case KEY_F12:       return VK_F12;
  }
  return -1;
}

bool internal_os_is_key_pressed(s32 key)
{
  return GetAsyncKeyState(key) & 0x01;
}

bool internal_os_is_key_down(s32 key)
{
  return GetAsyncKeyState(key) & 0x8000;
}

bool is_key_pressed(Key key)
{
  return internal_os_is_key_pressed(internal_convert_key_to_os_key(key));
}

bool is_key_down(Key key)
{
  return internal_os_is_key_down(internal_convert_key_to_os_key(key));
}

struct Window
{
  HWND handle;
};

enum WindowMessageType : u8
{
  MSG_NONE,
  MSG_QUIT,
};

bool poll_window_message(WindowMessageType *window_message_type)
{
  *window_message_type = MSG_NONE;

  MSG msg;
  if(!PeekMessage(&msg, null, 0, 0, PM_REMOVE)) return false;

  TranslateMessage(&msg);
  DispatchMessage(&msg);

  if(msg.message == WM_QUIT) *window_message_type = MSG_QUIT;

  return true;
}

LRESULT window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
  switch(msg)
  {
    case WM_CLOSE:
    {
      DestroyWindow(hwnd);
    }
    break;
    case WM_DESTROY:
    {
      PostQuitMessage(0);
    }
    break;
  }

  return DefWindowProc(hwnd, msg, w_param, l_param);
}

Window create_window(char *title, u32 width, u32 height)
{
  WNDCLASS window_class      = {};
  window_class.lpfnWndProc   = window_proc;
  window_class.lpszClassName = title;
  window_class.hCursor       = LoadCursor(null, IDC_ARROW);

  RegisterClass(&window_class);

  s32 monitor_width  = GetSystemMetrics(SM_CXSCREEN);
  s32 monitor_height = GetSystemMetrics(SM_CYSCREEN);

  s32 half_monitor_width  = monitor_width  / 2;
  s32 half_monitor_height = monitor_height / 2;

  s32 half_window_width  = width  / 2;
  s32 half_window_height = height / 2;

  s32 window_x_position = half_monitor_width  - half_window_width;
  s32 window_y_position = half_monitor_height - half_window_height;

  HWND window_handle = CreateWindow
  (
    title, title, WS_POPUP | WS_VISIBLE, 
    window_x_position, window_y_position, width, height, 
    null, null, null, null
  );

  Window window = {};
  window.handle = window_handle;

  return window;
}

struct Pipeline
{
  ID3D11Device *device;
  IDXGISwapChain *swap_chain;
  ID3D11DeviceContext *device_context;
  ID3D11RenderTargetView *rtv;
  D3D11_VIEWPORT viewport;
  ID3D11VertexShader *vs;
  ID3D11PixelShader *ps;
  ID3D11BlendState *blend_state;
  ID3D11SamplerState *point_sampler;
};

Pipeline init_gfx(Window window)
{
  Pipeline pipeline = {};

  //// Create Swap Chain, Device and Device Context ////
  DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
  swap_chain_desc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.SampleDesc.Count     = 1;
  swap_chain_desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.BufferCount          = 2;
  swap_chain_desc.OutputWindow         = window.handle;
  swap_chain_desc.Windowed             = true;
  swap_chain_desc.SwapEffect           = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  
  D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };

  u32 flags = D3D11_CREATE_DEVICE_DEBUG;
  
  D3D11CreateDeviceAndSwapChain(null, D3D_DRIVER_TYPE_HARDWARE, null, flags, feature_levels, ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &swap_chain_desc, &pipeline.swap_chain, &pipeline.device, null, &pipeline.device_context);

  //// Create Viewport ////
  pipeline.swap_chain->GetDesc(&swap_chain_desc);
  
  pipeline.viewport.Width    = (f32)swap_chain_desc.BufferDesc.Width;
  pipeline.viewport.Height   = (f32)swap_chain_desc.BufferDesc.Height;
  pipeline.viewport.MaxDepth = 1;

  //// Create RTV ////
  ID3D11Texture2D *rtv_texture;
  pipeline.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&rtv_texture);
  
  D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
  rtv_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  
  pipeline.device->CreateRenderTargetView(rtv_texture, &rtv_desc, &pipeline.rtv);

  char *shader_source = 
  R"(
    struct Vertex
    {
      float2 position;
      float2 uv;
    };

    static Vertex vertices[4] =
    {
      { float2(-1.0f,  1.0f), float2(0.0f, 0.0f) },  // Top-left
      { float2( 1.0f,  1.0f), float2(1.0f, 0.0f) },  // Top-right
      { float2(-1.0f, -1.0f), float2(0.0f, 1.0f) },  // Bottom-left
      { float2( 1.0f, -1.0f), float2(1.0f, 1.0f) },  // Bottom-right
    };

    struct VS_Output
    {
      float4 position : SV_POSITION;
      float2 uv       : TEXCOORD;
    };

    VS_Output vs_main(uint id : SV_VertexID)
    {
      VS_Output output;
      Vertex vertex   = vertices[id];
      output.position = float4(vertex.position, 0.0f, 1.0f);
      output.uv       = vertex.uv;
      return output;
    }

    Texture2D    main_texture : register(t0);
    SamplerState main_sampler : register(s0);

    float4 ps_main(VS_Output input) : SV_TARGET
    {
      return main_texture.Sample(main_sampler, input.uv).abgr; // TODO: Little endian shenanigans
    }
  )";
  s64 shader_size = strlen(shader_source);
  
  ID3DBlob *vs_blob;
  D3DCompile(shader_source, shader_size, null, null, null, "vs_main", "vs_5_0", 0, 0, &vs_blob, null);
  pipeline.device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), null, &pipeline.vs);

  ID3DBlob *ps_blob;
  D3DCompile(shader_source, shader_size, null, null, null, "ps_main", "ps_5_0", 0, 0, &ps_blob, null);
  pipeline.device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), null, &pipeline.ps);

  // TODO: Test without blending.
  D3D11_BLEND_DESC desc = {};
  desc.RenderTarget[0].BlendEnable           = true;
  desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
  desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
  desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
  desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
  desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
  desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
  desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  pipeline.device->CreateBlendState(&desc, &pipeline.blend_state);

  D3D11_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  pipeline.device->CreateSamplerState(&sampler_desc, &pipeline.point_sampler);

  return pipeline;
}

struct RenderBuffer
{
  ID3D11ShaderResourceView *texture_srv;
  ID3D11Texture2D *texture_2d;
  u32 *pixel_buffer;
  u32 width;
  u32 height;
  u32 area;

  f32 *z_buffer;
};

RenderBuffer create_main_buffer(Pipeline pipeline, u32 width, u32 height)
{
  RenderBuffer result = {};
  result.width  = width;
  result.height = height;
  result.area = width * height;

  u32 pitch = width * sizeof(u32);
  result.pixel_buffer = (u32*)calloc(1, pitch * height);
  result.z_buffer     = (f32*)calloc(1, pitch * height);

  D3D11_TEXTURE2D_DESC texture_desc = {};
  texture_desc.Width            = width;
  texture_desc.Height           = height;
  texture_desc.MipLevels        = 1;
  texture_desc.ArraySize        = 1;
  texture_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
  texture_desc.Usage            = D3D11_USAGE_DYNAMIC;
  texture_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
  texture_desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
  texture_desc.SampleDesc.Count = 1;

  D3D11_SUBRESOURCE_DATA subresource = {};
  subresource.pSysMem     = result.pixel_buffer;
  subresource.SysMemPitch = pitch;

  pipeline.device->CreateTexture2D(&texture_desc, &subresource, &result.texture_2d);
  pipeline.device->CreateShaderResourceView(result.texture_2d, null, &result.texture_srv);

  return result;
}

void clear_buffer(RenderBuffer *buffer)
{
  for(u32 y = 0; y < buffer->height; y++)
  {
    for(u32 x = 0; x < buffer->width; x++)
    {
      buffer->pixel_buffer[x + y * buffer->width] = 0x000000ff;
      buffer->z_buffer    [x + y * buffer->width] = 0.0f;
    }
  }
}

void draw_frame(Pipeline pipeline, RenderBuffer *buffer)
{
  D3D11_MAPPED_SUBRESOURCE mapped_resource;
  pipeline.device_context->Map(buffer->texture_2d, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
  
  u32 pitch = buffer->width * sizeof(u32);
  for(u32 y = 0; y < buffer->height; y++)
  {
    u8 *dest = (u8*)mapped_resource.pData + y * mapped_resource.RowPitch;
    u8 *src  = (u8*)buffer->pixel_buffer  + y * pitch;
    memcpy(dest, src, pitch);
  }
  
  pipeline.device_context->Unmap(buffer->texture_2d, 0);
}

void present_frame(Pipeline pipeline, RenderBuffer render_buffer)
{
  f32 background_color[4] = {1.0f, 0.0f, 1.0f, 1.0f};
  pipeline.device_context->ClearRenderTargetView(pipeline.rtv, background_color);

  pipeline.device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  pipeline.device_context->VSSetShader(pipeline.vs, null, 0);
  pipeline.device_context->PSSetShader(pipeline.ps, null, 0);
  pipeline.device_context->PSSetShaderResources(0, 1, &render_buffer.texture_srv);

  pipeline.device_context->RSSetViewports(1, &pipeline.viewport);
  pipeline.device_context->OMSetRenderTargets(1, &pipeline.rtv, null);
  pipeline.device_context->OMSetBlendState(pipeline.blend_state, null, 0xffffffff);

  // TODO: Added point sampler to test triangle overdraw
  pipeline.device_context->PSSetSamplers(0, 1, &pipeline.point_sampler);

  pipeline.device_context->Draw(4, 0);

  pipeline.swap_chain->Present(1, 0);
}