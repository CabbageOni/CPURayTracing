#pragma once

#include "vector.h"

union Ray
{
  struct
  {
    Vec3 origin, direction;
  };

  Ray(const Vec3& origin, const Vec3& direction);
};