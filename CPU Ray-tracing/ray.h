#pragma once

#include "vector.h"

union Ray
{
  struct
  {
    Vec3 origin, direction;
  };

  inline Ray() {}
  Ray(const Vec3& origin, const Vec3& direction);

  Vec3 at(float t) const;
};