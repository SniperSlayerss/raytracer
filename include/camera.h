#ifndef CAMERA_H
#define CAMERA_H

#include "raylib.h"
#include "raymath.h"

typedef struct {
  Vector3 camera_center;
  Vector3 viewport_u;
  Vector3 viewport_v;
  Vector3 pixel00_loc;
  Vector3 pixel_delta_u;
  Vector3 pixel_delta_v;
} camera_t;

void initialise_camera(int screen_width, int screen_height, camera_t *camera);

#endif
