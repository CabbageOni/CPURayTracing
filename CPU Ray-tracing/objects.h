#pragma once

#include "vector.h"
#include "ray.h"

struct HitRecord
{
  float t;
  Vec3 position;
  Vec3 normal;
};

struct Object
{
  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const = 0;
};

struct Sphere : public Object
{
  Vec3 center;
  float radius;

  inline Sphere() {}
  Sphere(const Vec3& center, float radius) : center(center), radius(radius) {}

  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const override;
};