#pragma once

#include <random>

#include "vector.h"

inline float uniform_rand()
{
  return float(rand() % RAND_MAX) / RAND_MAX;
}

inline Vec3 random_in_unit_sphere()
{
  //const float pi = 3.141592f;
  //float theta = uniform_rand() * 2 * pi, phi = (uniform_rand() - .5f) * pi;
  //
  //Vec3 direction = Vec3(cosf(theta) * sinf(phi), sinf(phi), sinf(theta) * cosf(phi));
  //return direction * uniform_rand();

  // original random distribution code
  Vec3 direction;
  do
  {
    direction = 2.0f * Vec3(uniform_rand(), uniform_rand(), uniform_rand()) - Vec3(1);
  } while (direction.lengthSqr() >= 1.0f);
  return direction;
}