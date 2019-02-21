#pragma once

#include "vector.h"
#include "ray.h"

#include <vector>
#include <typeinfo>

struct Material;

struct HitRecord
{
  float t;
  Vec3 position;
  Vec3 normal;
  Material* material_ptr;
};

struct AABB
{
  Vec3 pos_min, pos_max;

  AABB() {}
  AABB(const Vec3& pos_min, const Vec3& pos_max) : pos_min(pos_min), pos_max(pos_max) {}

  AABB operator+(const AABB& rhs) const;

  bool hit(const Ray& r, float t_min, float t_max) const;
};

struct Object
{
  virtual ~Object() {}
  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const = 0;
  virtual bool bounding_box(float t0, float t1, AABB& aabb) const = 0;
};

struct Sphere : public Object
{
  Vec3 center;
  float radius;
  Material* material_ptr = nullptr;

  inline Sphere() {}
  Sphere(const Vec3& center, float radius, Material* material_ptr) : center(center), radius(radius), material_ptr(material_ptr) {}
  ~Sphere();

  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const override;
  virtual bool bounding_box(float t0, float t1, AABB& aabb) const override;
};

struct MovingSphere : public Object
{
  Vec3 center_from, center_to;
  float time_from, time_to;
  float radius;
  Material* material_ptr = nullptr;

  inline MovingSphere() {}
  MovingSphere(const Vec3& center_from, const Vec3& center_to, float time_from, float time_to, float radius, Material* material_ptr) : 
    center_from(center_from), center_to(center_to), time_from(time_from), time_to(time_to), radius(radius), material_ptr(material_ptr) {}
  ~MovingSphere();

  inline Vec3 center(float time) const { return center_from + ((time - time_from) / (time_to - time_from))*(center_to - center_from); }
  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const override;
  virtual bool bounding_box(float t0, float t1, AABB& aabb) const override;
};

struct BVHnode : public Object
{
  Object* left = nullptr;
  Object* right = nullptr;
  bool child_is_obj = true;
  AABB aabb;

  BVHnode() {}
  BVHnode(std::vector<Object*>& objects, size_t from, size_t to, float t0, float t1);
  inline ~BVHnode()
  {
    if (!child_is_obj)
    {
      if (left)
        delete left;
      if (right)
        delete right;
    }
  }

  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const override;
  virtual bool bounding_box(float t0, float t1, AABB& aabb) const override;
};

// Materials

struct Material
{
  virtual bool scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const = 0;
};

struct Lambertian : public Material
{
  Color albedo;

  inline Lambertian() {}
  inline Lambertian(const Color& albedo) : albedo(albedo) {}

  virtual bool scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const override;
};

struct Metal : public Material
{
  Color albedo;
  float metallic;

  inline Metal() {}
  inline Metal(const Color& albedo, float metallic) : albedo(albedo), metallic(fminf(1, metallic)) { }

  virtual bool scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const override;
};

struct Dielectric : public Material
{
  float steepness;

  inline Dielectric() {}
  inline Dielectric(float steepness) : steepness(steepness) {}

  virtual bool scatter(const Ray& ray_in, const HitRecord& record, Vec3& attenuation, Ray& scattered) const override;
};