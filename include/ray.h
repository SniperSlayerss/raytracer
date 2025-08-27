#ifndef RAY_H
#define RAY_H
#include "raylib.h"
#include "raymath.h"

static inline Vector3 ray_at(Ray ray, double t) {
  return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
}

#endif
