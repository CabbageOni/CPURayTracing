#pragma once

#include "vector.h"

struct Ray
{
  Vec3 origin, direction;
  float time;

  inline Ray() {}
  inline Ray(const Vec3& origin, const Vec3& direction, float time) : origin(origin), direction(direction.normalized()), time(time) {}

  inline Vec3 at(float t) const { return origin + t * direction; }
};