#pragma once

#include <Windows.h>

extern struct WinAPI
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

  HDC hdc;
  HBITMAP hbm;
  HGLRC hglrc;
} winAPI;

extern struct Frame
{
  unsigned char* pixel_buffer = NULL;
  int width = 0;
  int height = 0;
  int stride = 0;
} frame;