#pragma once

#include <Windows.h>
#include <thread>
#include <mutex>
#include <atomic>

#include "vector.h"
#include "ray.h"

extern struct WinAPI
{
  HWND window_handle;
  HINSTANCE instance_handle;
  int initial_width = 500;
  int initial_height = 250;
  int width = initial_width;
  int height = initial_height;
  bool quit = false;

  RECT screen_client;
  RECT screen_window;

  HDC hdc;
  HBITMAP hbm;
  HGLRC hglrc;
} winAPI;

const float pi = 3.141592f;

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

class Camera
{
private:
  Vec3 bottom_left, horizontal, vertical;
  Vec3 up, right, look;
  Vec3 random_in_unit_disk();
  float focus_dist;
  float time_from, time_to;

public:
  Vec3 position;
  float lens_radius = 0;

  Camera(const Vec3& position, float theta, float phi, float focus_dist, float time_from, float time_to);

  Ray get_ray(float du, float dv);
};

extern struct ThreadData
{
  // non-shared
  std::thread thread_renderer;
  HDC temp_hdc;

  // shared
  std::atomic<bool> terminate_requested = false;
  std::mutex data_security;

  inline ~ThreadData()
  {
    terminate_requested = true;

    // if rendering, request termination
    if (thread_renderer.joinable())
      thread_renderer.join();
  }

} shared_thread_data;