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

Vec3 Vec3::normalized() const
{
  return *this / length();
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

Vec3 Vec3::reflect(const Vec3& in, const Vec3& axis)
{
  return in - 2 * dot(in, axis) * axis;
}

Vec3 operator*(float lhs, const Vec3& rhs)
{
  return rhs * lhs;
}