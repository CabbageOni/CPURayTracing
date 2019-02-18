#include <Windows.h>

#include "winAPI.h"

static HWND InitializeWindow(WNDCLASS& wnd_class, const HINSTANCE& h_inst);
static LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
static void UpdateClientRect(const HWND& hwnd);
static void Render(const HWND& hwnd);
static void ReallocateFrame(int new_width, int new_height, int new_stride);
static void DeallocateFrame();
static void ClearFrame(unsigned char clear_value);

struct WinAPI winAPI;
struct Frame frame;

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
  WNDCLASS window_class = {};
  winAPI.window_handle = InitializeWindow(window_class, h_instance);
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

HWND InitializeWindow(WNDCLASS& wnd_class, const HINSTANCE& h_inst)
{
  wnd_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wnd_class.lpfnWndProc = MainWindowCallback;
  wnd_class.hInstance = h_inst;
  wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  wnd_class.hbrBackground = (HBRUSH)(GetStockObject(GRAY_BRUSH));
  wnd_class.lpszClassName = "Framework for CPU Ray-Tracing";

  if (RegisterClass(&wnd_class))
    return CreateWindowEx(0,
      wnd_class.lpszClassName,
      "CPU Ray-Tracing",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT,
      winAPI.initial_width,
      winAPI.initial_height,
      0, 0, wnd_class.hInstance, 0);

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
  if (!winAPI.hdc)
  {
    winAPI.hdc = CreateCompatibleDC(temp_hdc);
    winAPI.hbm = CreateCompatibleBitmap(winAPI.hdc, winAPI.width, winAPI.height);
    SelectObject(winAPI.hdc, winAPI.hbm);
  
    BITMAP bmp;
    GetObject(winAPI.hbm, sizeof(BITMAP), &bmp);
    ReallocateFrame(bmp.bmWidth, bmp.bmHeight, bmp.bmWidthBytes);
    ClearFrame(0);
  }

  // RENDER START
  const int PIXEL_WIDTH = 4;
  const int INDEX_BLUE = 0;
  const int INDEX_GREEN = 1;
  const int INDEX_RED = 2;
  const int INDEX_ALPHA = 3;
  
  for (int i = 0; i < frame.height - 1; ++i)
  {
    for (int j = 0; j < frame.stride - 1; j += PIXEL_WIDTH)
    {
      frame.pixel_buffer[i * frame.stride + j + INDEX_RED] = 255;
      frame.pixel_buffer[i * frame.stride + j + INDEX_GREEN] = 0;
      frame.pixel_buffer[i * frame.stride + j + INDEX_BLUE] = 255;
    }
  }
  // RENDER END

  //BITMAPINFO info;
  //info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  //info.bmiHeader.biWidth = frame.width;
  //info.bmiHeader.biHeight = frame.height;
  //info.bmiHeader.biPlanes = 1;
  //info.bmiHeader.biBitCount = 8;
  //info.bmiHeader.biCompression = BI_RGB;
  //info.bmiHeader.biSizeImage = 0;
  //info.bmiHeader.biXPelsPerMeter = 0;
  //info.bmiColors[0].rgbReserved = 0;
  //info.bmiColors[0].rgbRed = 1;
  //info.bmiColors[0].rgbGreen = 1;
  //info.bmiColors[0].rgbBlue = 1;
  
  //SetDIBits(temp_hdc, winAPI.hbm, 0, frame.height, frame.pixel_buffer, &info, DIB_RGB_COLORS);

  //RECT rect;
  //GetClientRect(hwnd, &rect);
  //SetTextColor(temp_hdc, RGB(255, 0, 0));
  //DrawText(temp_hdc, "진짜 그지같은 WinAPI 뒤져라", -1, &rect, DT_SINGLELINE | DT_CENTER);

  auto ret = SetBitmapBits(winAPI.hbm, frame.stride * frame.height, frame.pixel_buffer);
  BitBlt(temp_hdc, 0, 0, frame.width, frame.height, winAPI.hdc, 0, 0, SRCCOPY);
  EndPaint(hwnd, &paint_struct);
}

void ReallocateFrame(int new_width, int new_height, int new_stride)
{
  delete[] frame.pixel_buffer;

  frame.pixel_buffer = new unsigned char[new_height * new_stride];

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
  for (int i = 0; i < frame.stride * frame.height; ++i)
    frame.pixel_buffer[i] = clear_value;
}