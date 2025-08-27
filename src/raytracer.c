#include "camera.h"
#include "ray.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

int main() {
  // Screen
  float aspect_ratio = 16.0f / 9.0f;
  int screen_width = 1000;

  // Calculate screen height, ensure at least 1
  int screen_height = (int)(screen_width / aspect_ratio);
  screen_height = (screen_height < 1) ? 1 : screen_height;

  jCamera camera = {0};
  initialise_camera(&camera, screen_width, screen_height);

  InitWindow(screen_width, screen_height, "Raytracer");

  // Load fragment shader
  Shader color_shader =
      LoadShader("shaders/vertex.glsl", "shaders/fragment.glsl");

  // Load compute shader
  int compute_shader = 0;
  unsigned int compute_texture = 0;
  char *shaderCode = LoadFileText("shaders/compute.glsl");
  int loc_time, loc_camera_center, loc_viewport_u, loc_viewport_v,
      loc_pixel00_loc, loc_pixel_delta_u, loc_pixel_delta_v, loc_screen_width,
      loc_screen_height;
  if (shaderCode) {
    int shaderData = rlCompileShader(shaderCode, RL_COMPUTE_SHADER);
    compute_shader = rlLoadComputeShaderProgram(shaderData);
    UnloadFileText(shaderCode);

    loc_time = rlGetLocationUniform(compute_shader, "time");
    loc_screen_width = rlGetLocationUniform(compute_shader, "screen_width");
    loc_screen_height = rlGetLocationUniform(compute_shader, "screen_height");
    if (compute_shader != 0) {
      Image blankImage = GenImageColor(screen_width, screen_height, BLANK);
      compute_texture =
          rlLoadTexture(blankImage.data, screen_width, screen_height,
                        RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
      UnloadImage(blankImage);

      loc_camera_center = rlGetLocationUniform(compute_shader, "camera_center");
      loc_viewport_u = rlGetLocationUniform(compute_shader, "viewport_u");
      loc_viewport_v = rlGetLocationUniform(compute_shader, "viewport_v");
      loc_pixel00_loc = rlGetLocationUniform(compute_shader, "pixel00_loc");
      loc_pixel_delta_u = rlGetLocationUniform(compute_shader, "pixel_delta_u");
      loc_pixel_delta_v = rlGetLocationUniform(compute_shader, "pixel_delta_v");

      loc_time = rlGetLocationUniform(compute_shader, "time");
      loc_screen_width = rlGetLocationUniform(compute_shader, "screen_width");
      loc_screen_height = rlGetLocationUniform(compute_shader, "screen_height");
    }
  }

  Mesh quad = GenMeshPlane(2.0f, 2.0f, 1, 1);
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    if (compute_shader != 0) {

      rlEnableShader(compute_shader);

      rlSetUniform(loc_camera_center, &camera.camera_center.x,
                   SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_viewport_u, &camera.viewport_u.x, SHADER_UNIFORM_VEC3,
                   1);
      rlSetUniform(loc_viewport_v, &camera.viewport_v.x, SHADER_UNIFORM_VEC3,
                   1);
      rlSetUniform(loc_pixel00_loc, &camera.pixel00_loc.x, SHADER_UNIFORM_VEC3,
                   1);
      rlSetUniform(loc_pixel_delta_u, &camera.pixel_delta_u.x,
                   SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_pixel_delta_v, &camera.pixel_delta_v.x,
                   SHADER_UNIFORM_VEC3, 1);

      rlSetUniform(loc_screen_width, &screen_width, SHADER_UNIFORM_INT, 1);
      rlSetUniform(loc_screen_height, &screen_height, SHADER_UNIFORM_INT, 1);

      float t = GetTime();
      rlSetUniform(loc_time, &t, SHADER_UNIFORM_FLOAT, 1);

      rlBindImageTexture(compute_texture, 0,
                         RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);
      rlComputeShaderDispatch((screen_width + 7) / 8, (screen_height + 7) / 8,
                              1);
      rlDisableShader();
    }

    BeginDrawing();
    ClearBackground(BLACK);

    if (compute_shader != 0) {
      Rectangle source = {0, 0, screen_width, -screen_height};
      Rectangle dest = {0, 0, screen_width, screen_height};
      DrawTexturePro((Texture2D){compute_texture, screen_width, screen_height,
                                 1, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8},
                     source, dest, (Vector2){0, 0}, 0, WHITE);
    }

    DrawFPS(10, 10);
    EndDrawing();
  }

  UnloadShader(color_shader);
  UnloadMesh(quad);
  if (compute_shader != 0)
    rlUnloadShaderProgram(compute_shader);
  if (compute_texture != 0)
    rlUnloadTexture(compute_texture);
  CloseWindow();
  return 0;
}
