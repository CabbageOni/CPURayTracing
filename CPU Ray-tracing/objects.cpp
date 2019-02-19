#include <math.h>

#include "objects.h"

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
      Vec3 test = record.normal.normalized();
      return true;
    }
    t = (-b + sqrtf(b * b - a * c)) / a;
    if (t < t_max && t > t_min)
    {
      record.t = t;
      record.position = r.at(t);
      record.normal = (record.position - center) / radius;
      return true;
    }
  }
  return false;
}