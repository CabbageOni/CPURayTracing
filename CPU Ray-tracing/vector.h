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

  Vec3(float uniform);
  Vec3(float x, float y, float z);

  float length() const;
  Vec3 normalized() const;

  Vec3 operator*(float rhs) const;
  Vec3 operator+(const Vec3& rhs) const;
  Vec3 operator/(float rhs) const;
};

Vec3 operator*(float lhs, const Vec3& rhs);