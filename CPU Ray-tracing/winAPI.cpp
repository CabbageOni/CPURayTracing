#include <Windows.h>
#include <math.h>
#include <vector>
#include <stdlib.h>
#include <thread>
#include <mutex>

#include "winAPI.h"
#include "vector.h"
#include "ray.h"
#include "objects.h"

#if defined(DEBUG) | defined(_DEBUG)
#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define DEBUG_LEAK_CHECKS(x) \
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);\
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);\
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);\
  _CrtSetBreakAlloc((x));
#else
#define DEBUG_LEAK_CHECKS(x)
#endif

using Time = LONGLONG;

static LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
static Time CurrentTime();
static float ElapsedTime(Time from, Time to);
static HWND InitializeWindow(const HINSTANCE& h_inst);
static void UpdateClientRect(const HWND& hwnd);
static void Render(const HWND& hwnd);
static void RequestThreadRendering(HDC& temp_hdc, unsigned char clear_value);
static void thread_renderer();

struct WinAPI winAPI;
struct Frame shared_frame;
struct ThreadData shared_thread_data;

static LARGE_INTEGER fixed_frequency;
const float framerate_target_dt = .016f;

int CALLBACK WinMain(HINSTANCE h_instance, HINSTANCE, LPSTR, int)
{
  DEBUG_LEAK_CHECKS(-1);

  winAPI.window_handle = InitializeWindow(h_instance);
  winAPI.instance_handle = h_instance;
  if (!winAPI.window_handle) return 0;

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
    RequestThreadRendering(temp_hdc, 150);

  SetBitmapBits(winAPI.hbm, shared_frame.stride * shared_frame.height, shared_frame.pixel_buffer);
  BitBlt(temp_hdc, 0, 0, shared_frame.width, shared_frame.height, winAPI.hdc, 0, 0, SRCCOPY);
  EndPaint(hwnd, &paint_struct);
}

void RequestThreadRendering(HDC& temp_hdc, unsigned char clear_value)
{
  winAPI.hdc = CreateCompatibleDC(temp_hdc);
  winAPI.hbm = CreateCompatibleBitmap(temp_hdc, winAPI.width, winAPI.height);
  SelectObject(winAPI.hdc, winAPI.hbm);

  BITMAP bmp;
  GetObject(winAPI.hbm, sizeof(BITMAP), &bmp);

  if (bmp.bmWidth < 2 || bmp.bmHeight < 2)
    return;

  //check if window is only moved
  if (shared_frame.width == bmp.bmWidth &&
      shared_frame.height == bmp.bmHeight &&
      shared_frame.stride == bmp.bmWidthBytes)
    return;

  // requests current rendering termination
  shared_thread_data.terminate_requested = true;

  // wait until rendering is terminated
  if (shared_thread_data.thread_renderer.joinable())
    shared_thread_data.thread_renderer.join();

  // reallocate the frame buffer
  shared_thread_data.data_security.lock();

  delete[] shared_frame.pixel_buffer;

  shared_frame.pixel_buffer = new Pixel[bmp.bmHeight * bmp.bmWidthBytes / 4];

  shared_frame.width = bmp.bmWidth;
  shared_frame.height = bmp.bmHeight;
  shared_frame.stride = bmp.bmWidthBytes;

  // clear the frame
  for (int i = 0; i < shared_frame.width * shared_frame.height; ++i)
    shared_frame.pixel_buffer[i] = { clear_value, clear_value, clear_value, 255 };

  shared_thread_data.data_security.unlock();

  shared_thread_data.terminate_requested = false;

  // run rendering
  shared_thread_data.thread_renderer = std::thread(thread_renderer);
}

float uniform_rand()
{
  return float(rand() % RAND_MAX) / RAND_MAX;
}

Vec3 random_in_unit_sphere()
{
  Vec3 position;
  do
  {
    position = 2.0f * Vec3(uniform_rand(), uniform_rand(), uniform_rand()) - Vec3(1);
  } while (position.lengthSqr() >= 1.0f);
  return position;
}

Color compute_raycast(const std::vector<Object*>& objects, const Ray& r)
{
  const float t_min = 0.001f;
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
  {
    Vec3 target = record.position + record.normal + random_in_unit_sphere();
    return .5f * compute_raycast(objects, Ray(record.position, target - record.position));
  }

  Vec3 unit_dir = r.direction.normalized();
  float t = .5f * (unit_dir.y + 1.0f);
  return (1.0f - t) * Vec3(1) + t * Vec3(.5f, .7f, 1.0f);
}

void thread_renderer()
{
  Vec3 camera_pos(0.0f);

  Vec3 bottom_left, horizontal, vertical;
  float resolution_ratio = shared_frame.width / float(shared_frame.height);
  if (resolution_ratio > 1) // width is wider
  {
    bottom_left = { -resolution_ratio, -1, 1 };
    horizontal = { resolution_ratio * 2, 0, 0 };
    vertical = { 0, 2, 0 };
  }
  else // height is wider
  {
    resolution_ratio = shared_frame.height / float(shared_frame.width);
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

  shared_thread_data.data_security.lock();
  for (int h = 0; h < shared_frame.height; ++h)
  {
    if (shared_thread_data.terminate_requested)
    {
      shared_thread_data.data_security.unlock();
      return;
    }

    for (int w = 0; w < shared_frame.width; ++w)
    {
      Color pixel_color(0);

      const int sample_count = 50;
      for (int sample_iter = 0; sample_iter < sample_count; ++sample_iter)
      {
        float du = (w + uniform_rand()) / float(shared_frame.width);
        float dv = (shared_frame.height - h + uniform_rand()) / float(shared_frame.height);
        Ray r(camera_pos, bottom_left + du * horizontal + dv * vertical);

        pixel_color += compute_raycast(objects, r);
      }
      pixel_color /= float(sample_count);
      pixel_color = Vec3(sqrtf(pixel_color.x), sqrtf(pixel_color.y), sqrtf(pixel_color.z));

      shared_frame.pixel_buffer[h * shared_frame.width + w].r = int(255.99 * pixel_color.r);
      shared_frame.pixel_buffer[h * shared_frame.width + w].g = int(255.99 * pixel_color.g);
      shared_frame.pixel_buffer[h * shared_frame.width + w].b = int(255.99 * pixel_color.b);
    }
  }
  shared_thread_data.data_security.unlock();
}