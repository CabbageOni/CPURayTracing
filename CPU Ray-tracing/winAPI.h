#pragma once

#include <Windows.h>
#include <thread>
#include <mutex>

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

union Pixel
{
  unsigned char data[4];
  struct
  {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
  };
};

extern struct Frame
{
  Pixel* pixel_buffer = nullptr;
  int width = 0;
  int height = 0;
  int stride = 0;

  inline ~Frame()
  {
    if (pixel_buffer)
      delete[] pixel_buffer;
    pixel_buffer = nullptr;
  }

} shared_frame;

extern struct ThreadData
{
  // non-shared
  std::thread thread_renderer;
  HDC temp_hdc;

  // shared
  bool terminate_requested = false;
  std::mutex data_security;
  std::mutex terminate_request_security;

  inline ~ThreadData()
  {
    terminate_request_security.lock();
    terminate_requested = true;
    terminate_request_security.unlock();

    // if rendering, request termination
    if (thread_renderer.joinable())
      thread_renderer.join();
  }

} shared_thread_data;