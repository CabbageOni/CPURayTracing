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
#include "randoms.h"

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
const float t_min = 0.001f;
const float t_max = 100000;

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

Camera::Camera(const Vec3& position, float theta, float phi, float focus_dist, float time_from, float time_to) : position(position), focus_dist(focus_dist), time_from(time_from), time_to(time_to)
{
  look = { cosf(phi) * cosf(theta), tanf(phi), cosf(phi) * sinf(theta) };

  float tan_half_fov = tanf(pi / 12) * focus_dist;
  float resolution_ratio = shared_frame.height / float(shared_frame.width);

  Vec3 quad_center = position + look * focus_dist;
  Vec3 right_hand = Vec3(look.z, 0, -look.x);

  right = Vec3::cross(Vec3(0,1,0), look);
  up = Vec3::cross(look, right);

  if (resolution_ratio < 1)
  {
    horizontal = Vec3::cross(Vec3(0, 1, 0), look) * tan_half_fov * 2 / resolution_ratio;
    vertical = Vec3::cross(look, right_hand) * tan_half_fov * 2;
  }
  else
  {
    horizontal = Vec3::cross(Vec3(0, 1, 0), look) * tan_half_fov * 2;
    vertical = Vec3::cross(look, right_hand) * tan_half_fov * 2 * resolution_ratio;
  }
  bottom_left = quad_center - horizontal * .5f - vertical * .5f;
}

bool BoundingBox(const std::vector<Object*>& objects, float t0, float t1, AABB& aabb)
{
  if (objects.size() < 1) return false;
  AABB temp_aabb;
  bool first_true = objects[0]->bounding_box(t0, t1, temp_aabb);
  if (!first_true) return false;
  else aabb = temp_aabb;

  for (int i = 1; i < objects.size(); ++i)
    if (objects[i]->bounding_box(t0, t1, temp_aabb))
      aabb = aabb + temp_aabb;

  return true;
}

Vec3 Camera::random_in_unit_disk()
{
  Vec3 p;
  do
  {
    p = 2.0f * Vec3(uniform_rand(), uniform_rand(), 0) - Vec3(1, 0, 0);
  } while (Vec3::dot(p, p) >= 1.0f);
  return p;
}

Ray Camera::get_ray(float du, float dv)
{
  Vec3 rd = lens_radius * random_in_unit_disk();
  Vec3 offset = right * rd.x + up * rd.y;
  return Ray(position + offset,
    bottom_left + du * horizontal + dv * vertical - position - offset,
    time_from + uniform_rand() * (time_to - time_from));
}

Color compute_raycast(BVHnode& bvhnode, const Ray& r, int depth)
{
  HitRecord record;
  bool is_hit = bvhnode.hit(r, t_min, t_max, record);

  if (is_hit)
  {
    Ray scattered;
    Color attenuation;
    if (depth < 50 && record.material_ptr->scatter(r, record, attenuation, scattered))
      return attenuation * compute_raycast(bvhnode, scattered, depth + 1);
    return Vec3(0);
  }

  Vec3 unit_dir = r.direction.normalized();
  float t = .5f * (unit_dir.y + 1.0f);
  return (1.0f - t) * Vec3(1) + t * Vec3(.5f, .7f, 1.0f);
}

void thread_renderer()
{
  int n = 204;
  std::vector<Object*> objects;
  objects.reserve(n);
  objects.push_back(new Sphere(Vec3(0,-1000,0), 1000, new Lambertian(Vec3(0.5f))));
   
  for(int i = 0; i < 200; ++i)
  {
    float choose_mat = uniform_rand();
    Vec3 center(-9 + 18 * uniform_rand(), 0.2f, -9 + 18 * uniform_rand());
      
    if (choose_mat < .7f)
    {
      objects.push_back(new MovingSphere(center, center + Vec3(0,.2f, 0), 0, 1, 0.2f, new Lambertian(Vec3(
        uniform_rand()*uniform_rand(), uniform_rand()*uniform_rand(), uniform_rand()*uniform_rand()
      ))));
    }
    else if (choose_mat < 0.85f)
      objects.push_back(new Sphere(center, 0.2f, new Metal(Vec3(
        0.5f * (1 + uniform_rand()), 0.5f * (1 + uniform_rand()), 0.5f * (1 + uniform_rand())
      ), 0.5f * uniform_rand())));
    else
      objects.push_back(new Sphere(center, .2f, new Dielectric(1.5f)));
  }
  
  objects.push_back(new Sphere(Vec3(0, 1, 0), 1.0f, new Dielectric(1.5f)));
  objects.push_back(new Sphere(Vec3(-4, 1, 0), 1.0f, new Lambertian(Vec3(.4f, .2f, .1f))));
  objects.push_back(new Sphere(Vec3(4, 1, 0), 1.0f, new Metal(Vec3(.7f, .6f, .5f), 1)));
  
  BVHnode root(objects, 0, objects.size(), t_min, t_max);

  shared_thread_data.data_security.lock();
  Vec3 camera_pos(10, 2, -3);
  Camera camera(camera_pos, pi * .9f, -pi * .05f, 7, 0, 1);
  camera.lens_radius = 0.08f;

  for (int h = 0; h < shared_frame.height; ++h)
  {
    if (shared_thread_data.terminate_requested)
    {
      for (auto iter = objects.begin(); iter != objects.end(); ++iter)
        delete *iter;
      shared_thread_data.data_security.unlock();
      return;
    }

    for (int w = 0; w < shared_frame.width; ++w)
    {
      Color pixel_color(0);

      const int AA_sample_count = 2000;
      for (int AA_sample_iter = 0; AA_sample_iter < AA_sample_count; ++AA_sample_iter)
      {
        float du = (w + uniform_rand()) / float(shared_frame.width);
        float dv = (shared_frame.height - h + uniform_rand()) / float(shared_frame.height);

        pixel_color += compute_raycast(root, camera.get_ray(du, dv), 0);
      }
      pixel_color /= float(AA_sample_count);
      pixel_color = Vec3(sqrtf(pixel_color.x), sqrtf(pixel_color.y), sqrtf(pixel_color.z));

      shared_frame.pixel_buffer[h * shared_frame.width + w].r = int(255.99 * pixel_color.r);
      shared_frame.pixel_buffer[h * shared_frame.width + w].g = int(255.99 * pixel_color.g);
      shared_frame.pixel_buffer[h * shared_frame.width + w].b = int(255.99 * pixel_color.b);
    }
  }
  
  for (auto iter = objects.begin(); iter != objects.end(); ++iter)
    delete *iter;
  shared_thread_data.data_security.unlock();
}