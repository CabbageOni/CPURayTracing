#include <Windows.h>
#include <math.h>
#include <vector>
#include <stdlib.h>

#include "winAPI.h"
#include "vector.h"
#include "ray.h"
#include "objects.h"

using Time = LONGLONG;

static LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
static Time CurrentTime();
static float ElapsedTime(Time from, Time to);
static HWND InitializeWindow(const HINSTANCE& h_inst);
static void UpdateClientRect(const HWND& hwnd);
static void Render(const HWND& hwnd);
static void ReallocateFrame(int new_width, int new_height, int new_stride);
static void DeallocateFrame();
static void ClearFrame(unsigned char clear_value);
static void ResetRenderProgress();
static void RenderFrame();

struct WinAPI winAPI;
struct Frame frame;

static int draw_flag = 0;
static LARGE_INTEGER fixed_frequency;
const float framerate_target_dt = .016f;

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
  winAPI.window_handle = InitializeWindow(h_instance);
  winAPI.instance_handle = h_instance;
  if (!winAPI.window_handle) return 0;

  UpdateWindow(winAPI.window_handle);

  Time last_frame_time = CurrentTime();
  QueryPerformanceFrequency(&fixed_frequency);
  MSG msg;
  while (!winAPI.quit)
  {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    InvalidateRect(winAPI.window_handle, NULL, FALSE);

    Time current_time = CurrentTime();
    float dt = ElapsedTime(last_frame_time, current_time);
    int ms_to_sleep = (int)(framerate_target_dt * 1000.f) - (int)(dt * 1000.f + 2.f);
    if (ms_to_sleep > 0 && dt < framerate_target_dt)
      Sleep(ms_to_sleep);
    
    current_time = CurrentTime();
    dt = ElapsedTime(last_frame_time, current_time);
    while (dt < framerate_target_dt)
    {
      current_time = CurrentTime();
      dt = ElapsedTime(last_frame_time, current_time);
    }
    last_frame_time = current_time;
  }

  DeallocateFrame();
  return 0;
}

Time CurrentTime()
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result.QuadPart;
}

float ElapsedTime(Time from, Time to)
{
  static LARGE_INTEGER freq;
  return (float)(to - from) / (float)fixed_frequency.QuadPart;
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
  case WM_EXITSIZEMOVE:
    ResetRenderProgress();
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
    DeleteObject(winAPI.hbm);
    ReleaseDC(hwnd, winAPI.hdc);
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
    ReallocateFrame(bmp.bmWidth, bmp.bmHeight, bmp.bmWidthBytes);
    ClearFrame(150);
  }
  else if (frame.done)
  {
    EndPaint(hwnd, &paint_struct);
    return;
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
    frame.pixel_buffer[i] = { clear_value, clear_value, clear_value, clear_value };
}

static int h = 0;

void ResetRenderProgress()
{
  frame.done = false;
  h = 0;
}

float hit_sphere(const Vec3& center, float radius, const Ray& r)
{
  Vec3 diff = r.origin - center;
  float a = Vec3::dot(r.direction, r.direction);
  float b = 2.0f * Vec3::dot(diff, r.direction);
  float c = Vec3::dot(diff, diff) - radius * radius;
  float discriminant = b * b - 4 * a * c;

  return discriminant >= 0 ? (-b - sqrtf(discriminant)) / (2.0f * a) : -1.0f;
}

Color compute_raycast(const std::vector<Object*>& objects, const Ray& r)
{
  const float t_min = 0;
  const float t_max = 1000;

  HitRecord record;
  bool is_hit = false;
  float closest_t = t_max;
  for (auto iter = objects.begin(); iter != objects.end(); ++iter)
    if ((*iter)->hit(r, t_min, closest_t, record))
    {
      is_hit = true;
      closest_t = record.t;
    }

  if (is_hit)
    return .5f * (record.normal + Vec3(1));

  Vec3 unit_dir = r.direction.normalized();
  float t = .5f * (unit_dir.y + 1.0f);
  return (1.0f - t) * Vec3(1) + t * Vec3(.5f, .7f, 1.0f);
}

float uniform_rand()
{
  return rand() % 10000 / 10000.f;
}

void RenderFrame()
{
  const int iter_per_frame_count = 10;
  int iter_count = 0;

  Vec3 camera_pos(0.0f);

  Vec3 bottom_left, horizontal, vertical;
  float resolution_ratio = frame.width / float(frame.height);
  if (resolution_ratio > 1) // width is wider
  {
    bottom_left = { -resolution_ratio, -1, 1 };
    horizontal = { resolution_ratio * 2, 0, 0 };
    vertical = { 0, 2, 0 };
  }
  else // height is wider
  {
    resolution_ratio = frame.height / float(frame.width);
    bottom_left = { -1, -resolution_ratio, 1 };
    horizontal = { 2, 0, 0 };
    vertical = { 0, resolution_ratio * 2, 0 };
  }
  
  Sphere sphere(Vec3(0, 0, 1), .5f);
  Sphere sphere2(Vec3(0, -100.5f, 1), 100);
  std::vector<Object*> objects;
  objects.reserve(2);
  objects.push_back(&sphere);
  objects.push_back(&sphere2);

  for (; h < frame.height; ++h)
  {
    for (int w = 0; w < frame.width; ++w)
    {
      Color pixel_color(0);

      const int sample_count = 50;
      for (int sample_iter = 0; sample_iter < sample_count; ++sample_iter)
      {
        float du = (w + uniform_rand()) / float(frame.width);
        float dv = (frame.height - h + uniform_rand()) / float(frame.height);
        Ray r(camera_pos, bottom_left + du * horizontal + dv * vertical);

        pixel_color += compute_raycast(objects, r);
      }
      pixel_color /= float(sample_count);
  
      frame.pixel_buffer[h * frame.width + w].r = int(255.99 * pixel_color.r);
      frame.pixel_buffer[h * frame.width + w].g = int(255.99 * pixel_color.g);
      frame.pixel_buffer[h * frame.width + w].b = int(255.99 * pixel_color.b);
    }
    if (++iter_count > iter_per_frame_count)
      goto halted;
  }

  halted:
  if (h == frame.height)
    frame.done = true;
}