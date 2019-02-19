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