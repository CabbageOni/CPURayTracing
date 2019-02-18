#include <Windows.h>

#include "winAPI.h"

static HWND InitializeWindow(const HINSTANCE& h_inst);
static LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
static void UpdateClientRect(const HWND& hwnd);
static void Render(const HWND& hwnd);
static void ReallocateFrame(int new_width, int new_height, int new_stride);
static void DeallocateFrame();
static void ClearFrame(unsigned char clear_value);
static void RenderFrame();

struct WinAPI winAPI;
struct Frame frame;

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
  winAPI.window_handle = InitializeWindow(h_instance);
  winAPI.instance_handle = h_instance;
  if (!winAPI.window_handle) return 0;

  MSG msg;
  while (!winAPI.quit)
  {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  DeallocateFrame();
  return 0;
}

HWND InitializeWindow(const HINSTANCE& h_inst)
{
  WNDCLASSEX window_class_ex = {};
  window_class_ex.cbSize = sizeof(WNDCLASSEX);
  window_class_ex.style = CS_HREDRAW | CS_VREDRAW;
  window_class_ex.lpfnWndProc = MainWindowCallback;
  window_class_ex.cbClsExtra = 0;
  window_class_ex.cbWndExtra = 0;
  window_class_ex.hInstance = h_inst;
  window_class_ex.hCursor = LoadCursor(NULL, IDC_ARROW);
  window_class_ex.hbrBackground = (HBRUSH)(GetStockObject(GRAY_BRUSH));
  window_class_ex.lpszClassName = "Framework for CPU Ray-Tracing";

  if (RegisterClassEx(&window_class_ex))
    return CreateWindowEx(0,
      window_class_ex.lpszClassName,
      "CPU Ray-Tracing",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT,
      winAPI.initial_width,
      winAPI.initial_height,
      0, 0, window_class_ex.hInstance, 0);

  return nullptr;
}

LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
  switch (u_msg)
  {
  case WM_SIZE:
    GetClientRect(hwnd, &winAPI.screen_client);
    GetWindowRect(hwnd, &winAPI.screen_window);
    UpdateClientRect(hwnd);
    if (winAPI.hbm)
    {
      DeleteObject(winAPI.hbm);
      winAPI.hbm = NULL;
    }
    if (winAPI.hdc)
    {
      DeleteDC(winAPI.hdc);
      winAPI.hdc = NULL;
    }
    break;

  case WM_MOVE:
    GetClientRect(hwnd, &winAPI.screen_client);
    GetWindowRect(hwnd, &winAPI.screen_window);
    UpdateClientRect(hwnd);
    break;

  case WM_CLOSE:
  case WM_DESTROY:
    winAPI.quit = true;
    ReleaseDC(hwnd, winAPI.hdc);
    DeleteObject(winAPI.hbm);
    break;

  case WM_PAINT:
    Render(hwnd);
    break;

  case WM_ERASEBKGND:
    return 1;

  default:
    return DefWindowProc(hwnd, u_msg, w_param, l_param);
  }

  return 0;
}

void UpdateClientRect(const HWND& hwnd)
{
  POINT p;
  p.x = winAPI.screen_client.left;
  p.y = winAPI.screen_client.top;
  ClientToScreen(hwnd, &p);
  winAPI.screen_client.left = p.x;
  winAPI.screen_client.top = p.y;
  p.x = winAPI.screen_client.right;
  p.y = winAPI.screen_client.bottom;
  ClientToScreen(hwnd, &p);
  winAPI.screen_client.right = p.x;
  winAPI.screen_client.bottom = p.y;

  winAPI.width = winAPI.screen_client.right - winAPI.screen_client.left;
  winAPI.height = winAPI.screen_client.bottom - winAPI.screen_client.top;
}

void Render(const HWND& hwnd)
{
  PAINTSTRUCT paint_struct;
  HDC temp_hdc;

  temp_hdc = BeginPaint(hwnd, &paint_struct);
  if (!winAPI.hbm)
  {
    winAPI.hdc = CreateCompatibleDC(temp_hdc);
    winAPI.hbm = CreateCompatibleBitmap(temp_hdc, winAPI.width, winAPI.height);
    SelectObject(winAPI.hdc, winAPI.hbm);
  
    BITMAP bmp;
    GetObject(winAPI.hbm, sizeof(BITMAP), &bmp);
    bmp.bmBitsPixel = 8;
    ReallocateFrame(bmp.bmWidth, bmp.bmHeight, bmp.bmWidthBytes);
    ClearFrame(0);
  }

  RenderFrame();

  /*
  // disclaimer: tried to update SetBitmapBits into SetDIBits, but failed.

  BITMAPINFO info;
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = frame.width;
  info.bmiHeader.biHeight = frame.height;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 8;
  info.bmiHeader.biCompression = BI_RGB;
  
  SetDIBits(temp_hdc, winAPI.hbm, 0, frame.stride, frame.pixel_buffer, &info, DIB_RGB_COLORS);
  */

  SetBitmapBits(winAPI.hbm, frame.stride * frame.height, frame.pixel_buffer);
  BitBlt(temp_hdc, 0, 0, frame.width, frame.height, winAPI.hdc, 0, 0, SRCCOPY);
  EndPaint(hwnd, &paint_struct);
}

void ReallocateFrame(int new_width, int new_height, int new_stride)
{
  if (new_width < 2 || new_height < 2)
    return;

  delete[] frame.pixel_buffer;

  frame.pixel_buffer = new Pixel[new_height * new_stride / 4];

  frame.width = new_width;
  frame.height = new_height;
  frame.stride = new_stride;
}

void DeallocateFrame()
{
  delete[] frame.pixel_buffer;
  frame.pixel_buffer = NULL;
}

void ClearFrame(unsigned char clear_value)
{
  for (int i = 0; i < frame.width * frame.height; ++i)
    frame.pixel_buffer[i] = {0};
}

void RenderFrame()
{
  for (int i = 0; i < frame.height; ++i)
  {
    for (int j = 0; j < frame.width; ++j)
    {
      frame.pixel_buffer[i * frame.width + j].r = 255;
      frame.pixel_buffer[i * frame.width + j].g = 255;
      frame.pixel_buffer[i * frame.width + j].b = 0;
    }
  }
}