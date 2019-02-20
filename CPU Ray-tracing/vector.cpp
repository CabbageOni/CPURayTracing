#include <math.h>

#include "vector.h"

Vec3::Vec3(float uniform) : x(uniform), y(uniform), z(uniform) {}

Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

float Vec3::length() const
{
  return sqrtf(lengthSqr());
}

float Vec3::lengthSqr() const
{
  return x * x + y * y + z * z;
}

void Vec3::normalize()
{
  float l = length();

  x /= l;
  y /= l;
  z /= l;
}

Vec3 Vec3::normalized() const
{
  return *this / length();
}

Vec3 Vec3::operator-() const
{
  return *this * -1;
}

Vec3 Vec3::operator*(float rhs) const
{
  return Vec3(x * rhs, y * rhs, z * rhs);
}

Vec3 Vec3::operator*(const Vec3& rhs) const
{
  return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
}

Vec3 Vec3::operator+(const Vec3& rhs) const
{
  return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
}

Vec3 Vec3::operator-(const Vec3& rhs) const
{
  return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
}

Vec3 Vec3::operator/(float rhs) const
{
  return Vec3(x / rhs, y / rhs, z / rhs);
}

Vec3& Vec3::operator+=(const Vec3& rhs)
{
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
  return *this;
}

Vec3& Vec3::operator/=(float rhs)
{
  x /= rhs;
  y /= rhs;
  z /= rhs;
  return *this;
}

float Vec3::dot(const Vec3& lhs, const Vec3& rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 Vec3::cross(const Vec3& lhs, const Vec3& rhs)
{
  return Vec3(lhs.y * rhs.z - lhs.z * rhs.y, 
              lhs.z * rhs.x - lhs.x * rhs.z,
              lhs.x * rhs.y - lhs.y * rhs.x);
}

Vec3 Vec3::reflect(const Vec3& in, const Vec3& axis)
{
  return in - 2 * dot(in, axis) * axis;
}

bool Vec3::refract(const Vec3& in, const Vec3& axis, float steepness, Vec3& refracted)
{
  Vec3 uv = in.normalized();
  float dt = dot(uv, axis);
  float discriminant = 1.0f - steepness * steepness * (1 - dt * dt);
  if (discriminant > 0)
  {
    refracted = steepness * (uv - axis * dt) - axis * sqrtf(discriminant);
    return true;
  }
  return false;
}

Vec3 operator*(float lhs, const Vec3& rhs)
{
  return rhs * lhs;
}