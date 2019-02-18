#include <Windows.h>

#include "winAPI.h"

static HWND InitializeWindow(WNDCLASS& wnd_class, const HINSTANCE& h_inst);
static LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
static void UpdateClientRect(const HWND& hwnd);

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

  return 0;
}

HWND InitializeWindow(WNDCLASS& wnd_class, const HINSTANCE& h_inst)
{
  wnd_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wnd_class.lpfnWndProc = MainWindowCallback;
  wnd_class.hInstance = h_inst;
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
  case WM_SETCURSOR:
    static HCURSOR normal_cursor = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(normal_cursor);
    break;

  case WM_SIZE:
    GetClientRect(hwnd, &winAPI.screen_client);
    GetWindowRect(hwnd, &winAPI.screen_window);
    UpdateClientRect(hwnd);
    break;

  case WM_MOVE:
    GetClientRect(hwnd, &winAPI.screen_client);
    GetWindowRect(hwnd, &winAPI.screen_window);
    UpdateClientRect(hwnd);
    break;

  case WM_CLOSE:
  case WM_DESTROY:
    winAPI.quit = true;
    ReleaseDC(hwnd, winAPI.m_hdc);
    break;

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