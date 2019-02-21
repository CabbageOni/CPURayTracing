#pragma once

#include "vector.h"
#include "ray.h"

struct Material;

struct HitRecord
{
  float t;
  Vec3 position;
  Vec3 normal;
  Material* material_ptr;
};

struct Object
{
  virtual ~Object() {}
  virtual bool hit(const Ray& r, float t_min, float t_max, HitRecord& record) const = 0;
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
};

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