#pragma once

union Vec3
{
  float data[3];
  struct
  {
    float x, y, z;
  };
  struct
  {
    float r, g, b;
  };

  inline Vec3() {}
  Vec3(float uniform);
  Vec3(float x, float y, float z);

  float length() const;
  float lengthSqr() const;
  void normalize();
  Vec3 normalized() const;

  Vec3 operator-() const;

  Vec3 operator*(float rhs) const;
  Vec3 operator*(const Vec3& rhs) const;
  Vec3 operator+(const Vec3& rhs) const;
  Vec3 operator-(const Vec3& rhs) const;
  Vec3 operator/(float rhs) const;

  Vec3& operator+=(const Vec3& rhs);
  Vec3& operator/=(float rhs);
  
  static float dot(const Vec3& lhs, const Vec3& rhs);
  static Vec3 cross(const Vec3& lhs, const Vec3& rhs);
  static Vec3 reflect(const Vec3& in, const Vec3& axis);
  static bool refract(const Vec3& in, const Vec3& axis, float steepness, Vec3& refracted);
};

Vec3 operator*(float lhs, const Vec3& rhs);

using Color = Vec3;