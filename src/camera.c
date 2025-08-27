#include "camera.h"

void initialise_camera(int screen_width, int screen_height, camera_t *camera) {
  // Camera
  float focal_length = 1.0f;
  float viewport_height = 2.0f;
  float aspect_ratio = (float)screen_width / (float)screen_height;
  float viewport_width = viewport_height * aspect_ratio;
  Vector3 camera_center = {0, 0, 0};

  // Calculate the vectors across the horizontal and down the vertical viewport
  // edges.
  Vector3 viewport_u = {viewport_width, 0, 0};
  Vector3 viewport_v = {0, -viewport_height, 0};

  // Calculate the horizontal and vertical delta vectors from pixel to pixel.
  Vector3 pixel_delta_u = Vector3Scale(viewport_u, 1.0f / screen_width);
  Vector3 pixel_delta_v = Vector3Scale(viewport_v, 1.0f / screen_height);

  // Calculate the location of the upper left pixel.
  Vector3 fl_v = {0, 0, focal_length};
  Vector3 viewport_upper_left =
      Vector3Subtract(Vector3Subtract(Vector3Subtract(camera_center, fl_v),
                                      Vector3Scale(viewport_u, 0.5f)),
                      Vector3Scale(viewport_v, 0.5f));
  Vector3 pixel00_loc =
      Vector3Add(viewport_upper_left,
                 Vector3Scale(Vector3Add(pixel_delta_u, pixel_delta_v), 0.5f));

  camera->camera_center = camera_center;
  camera->viewport_u = viewport_u;
  camera->viewport_v = viewport_v;
  camera->pixel00_loc = pixel00_loc;
  camera->pixel_delta_u = pixel_delta_u;
  camera->pixel_delta_v = pixel_delta_v;
}
