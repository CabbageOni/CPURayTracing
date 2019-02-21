#include <math.h>

#include "objects.h"
#include "randoms.h"

#include <vector>
#include <algorithm>

AABB AABB::operator+(const AABB& rhs) const
{
  return AABB(
    Vec3(
    fminf(pos_min.x, rhs.pos_min.x),
    fminf(pos_min.y, rhs.pos_min.y),
    fminf(pos_min.z, rhs.pos_min.z)),
    Vec3(
      fmaxf(pos_max.x, rhs.pos_max.x),
      fmaxf(pos_max.y, rhs.pos_max.y),
      fmaxf(pos_max.z, rhs.pos_max.z)));
}

bool AABB::hit(const Ray& r, float t_min, float t_max) const
{  
  for (int i = 0; i < 3; ++i)
  {
    float invD = 1 / r.direction[i];
    float t0 = (pos_min[i] - r.origin[i]) * invD;
    float t1 = (pos_max[i] - r.origin[i]) * invD;
    if (invD < 0)
      std::swap(t0, t1);
    t_min = t0 > t_min ? t0 : t_min;
    t_max = t1 < t_max ? t1 : t_max;

    if (t_max <= t_min)
      return false;
  }
  return true;
}

Sphere::~Sphere()
{
  if (material_ptr)
    delete material_ptr;
}

bool Sphere::hit(const Ray& r, float t_min, float t_max, HitRecord& record) const
{
  Vec3 diff = r.origin - center;
  float a = Vec3::dot(r.direction, r.direction);
  float b = Vec3::dot(diff, r.direction);
  float c = Vec3::dot(diff, diff) - radius * radius;
  float discriminant = b * b - a * c;

  if (discriminant > 0)
  {
    float t = (-b - sqrtf(b * b - a * c)) / a;
    if (t < t_max && t > t_min)
    {
      record.t = t;
      record.position = r.at(t);
      record.normal = (record.position - center) / radius;
      record.material_ptr = material_ptr;
      Vec3 test = record.normal.normalized();
      return true;
    }
    t = (-b + sqrtf(b * b - a * c)) / a;
    if (t < t_max && t > t_min)
    {
      record.t = t;
      record.position = r.at(t);
      record.normal = (record.position - center) / radius;
      record.material_ptr = material_ptr;
      return true;
    }
  }
  return false;
}

bool Sphere::bounding_box(float t0, float t1, AABB& aabb) const
{
  aabb = AABB(center - Vec3(radius), center + Vec3(radius));
  return true;
}

MovingSphere::~MovingSphere()
{
  if (material_ptr)
    delete material_ptr;
}

BVHnode::BVHnode(std::vector<Object*>& objects, size_t from, size_t to, float t0, float t1)
{
  int axis = int(3 * uniform_rand());
  std::sort(objects.begin() + from, objects.begin() + to, 
    [axis](const Object* left, const Object* right)
  {
    AABB aabb_left, aabb_right;
    left->bounding_box(0, 0, aabb_left);
    right->bounding_box(0, 0, aabb_right);

    return aabb_left.pos_min[axis] - aabb_right.pos_min[axis] > 0;
  });

  if (to - from == 1)
  {
    left = right = objects[from];
    child_is_obj = true;
  }
  else if (to - from == 2)
  {
    left = objects[from];
    right = objects[from + 1];
    child_is_obj = true;
  }
  else
  {
    left = new BVHnode(objects, from, from + (to - from) / 2, t0, t1);
    right = new BVHnode(objects, from + (to - from) / 2, to, t0, t1);
    child_is_obj = false;
  }

  AABB aabb_left, aabb_right;
  left->bounding_box(t0, t1, aabb_left);
  right->bounding_box(t0, t1, aabb_right);

  aabb = aabb_left + aabb_right;
}

bool BVHnode::hit(const Ray& r, float t_min, float t_max, HitRecord& record) const
{
  if (aabb.hit(r, t_min, t_max))
  {
    HitRecord left_record, right_record;
    bool hit_left = left->hit(r, t_min, t_max, left_record);
    bool hit_right = right->hit(r, t_min, t_max, right_record);
    if (hit_left && hit_right)
      record = left_record.t < right_record.t ? left_record : right_record;
    else if (hit_left)
      record = left_record;
    else if (hit_right)
      record = right_record;
    return hit_left || hit_right;
  }
  else return false;
}

bool BVHnode::bounding_box(float t0, float t1, AABB& aabb) const
{
  aabb = this->aabb;
  return true;
}

bool MovingSphere::hit(const Ray& r, float t_min, float t_max, HitRecord& record) const
{
  Vec3 diff = r.origin - center(r.time);
  float a = Vec3::dot(r.direction, r.direction);
  float b = Vec3::dot(diff, r.direction);
  float c = Vec3::dot(diff, diff) - radius * radius;
  float discriminant = b * b - a * c;

  if (discriminant > 0)
  {
    float t = (-b - sqrtf(b * b - a * c)) / a;
    if (t < t_max && t > t_min)
    {
      record.t = t;
      record.position = r.at(t);
      record.normal = (record.position - center(r.time)) / radius;
      record.material_ptr = material_ptr;
      Vec3 test = record.normal.normalized();
      return true;
    }
    t = (-b + sqrtf(b * b - a * c)) / a;
    if (t < t_max && t > t_min)
    {
      record.t = t;
      record.position = r.at(t);
      record.normal = (record.position - center(r.time)) / radius;
      record.material_ptr = material_ptr;
      return true;
    }
  }
  return false;
}

bool MovingSphere::bounding_box(float t0, float t1, AABB& aabb) const
{
  AABB aabb_min(center_from - Vec3(radius), center_from + Vec3(radius));
  AABB aabb_max(center_to - Vec3(radius), center_to + Vec3(radius));

  aabb = aabb_min + aabb_max;
  return true;
}

bool Lambertian::scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const
{
  Vec3 target = record.position + record.normal + random_in_unit_sphere();
  scattered = Ray(record.position, target - record.position, ray_in.time);
  attenuation = albedo;
  return true;
}

bool Metal::scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const
{
  Vec3 reflected = Vec3::reflect(ray_in.direction.normalized(), record.normal);
  scattered = Ray(record.position, reflected + (1 - metallic) * random_in_unit_sphere(), ray_in.time);
  attenuation = albedo;
  return Vec3::dot(scattered.direction, record.normal) > 0;
}

static float schlick(float cos, float steepness)
{
  float r = (1 - steepness) / (1 + steepness);
  r *= r;
  return r + (1 - r) * powf(1 - cos, 5);
}

bool Dielectric::scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const
{
  Vec3 outward_normal;
  Vec3 reflected = Vec3::reflect(ray_in.direction, record.normal);
  float correct_steepness;
  attenuation = Vec3(1);
  Vec3 refracted;
  float reflect_prob;
  float cos;
  float dot_ray_normal = Vec3::dot(ray_in.direction, record.normal);

  if (dot_ray_normal > 0)
  {
    outward_normal = -record.normal;
    correct_steepness = steepness;
    cos = steepness * dot_ray_normal / ray_in.direction.length();
  }
  else
  {
    outward_normal = record.normal;
    correct_steepness = 1.0f / steepness;
    cos = -dot_ray_normal / ray_in.direction.length();
  }
  if (Vec3::refract(ray_in.direction, outward_normal, correct_steepness, refracted))
    reflect_prob = schlick(cos, steepness);
  else
  {
    reflect_prob = 1;
  }
  if (uniform_rand() < reflect_prob)
    scattered = Ray(record.position, reflected, ray_in.time);
  else
    scattered = Ray(record.position, refracted, ray_in.time);

  return true;
}
