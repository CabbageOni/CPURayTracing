#include <math.h>

#include "objects.h"
#include "randoms.h"

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

bool Lambertian::scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const
{
  Vec3 target = record.position + record.normal + random_in_unit_sphere();
  scattered = Ray(record.position, target - record.position);
  attenuation = albedo;
  return true;
}

bool Metal::scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const
{
  Vec3 reflected = Vec3::reflect(ray_in.direction.normalized(), record.normal);
  scattered = Ray(record.position, reflected + (1 - metallic) * random_in_unit_sphere());
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
    scattered = Ray(record.position, reflected);
  else
    scattered = Ray(record.position, refracted);

  return true;
}