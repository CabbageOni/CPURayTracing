#pragma once

#include <Windows.h>

struct WinAPI
{
  HWND window_handle;
  HINSTANCE instance_handle;
  int initial_width = 1080;
  int initial_height = 720;
  int width = initial_width;
  int height = initial_height;
  bool quit = false;

  RECT screen_client;
  RECT screen_window;

  HDC m_hdc;
  HGLRC m_hglrc;
} winAPI;