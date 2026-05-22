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

bool os_is_key_pressed(s32 key)
{
  return GetAsyncKeyState(key) & 0x01;
}

struct Window
{
  HWND handle;
};

void window_message_handler(bool *running)
{
  MSG msg;
  while(PeekMessage(&msg, null, 0, 0, PM_REMOVE))
  {
    switch(msg.message)
    {
      case WM_QUIT:
      {
        *running = false;
      }
      break;
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
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
      Vertex vertex     = vertices[id];
      output.position   = float4(vertex.position, 0.0f, 1.0f);
      output.uv         = vertex.uv;
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
};

RenderBuffer create_main_buffer(Pipeline pipeline, u32 width, u32 height)
{
  RenderBuffer result = {};
  result.width  = width;
  result.height = height;
  result.area = width * height;

  u32 pitch = width * sizeof(u32);
  result.pixel_buffer = (u32*)calloc(1, pitch * height);

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
    }
  }
}

void present_frame(Pipeline pipeline, RenderBuffer *buffer)
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

void render_frame(Pipeline pipeline, RenderBuffer render_buffer)
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

  pipeline.device_context->Draw(4, 0);

  pipeline.swap_chain->Present(1, 0);
}