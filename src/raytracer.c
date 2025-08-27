#include "camera.h"
#include "hittable.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "syslog.h"
#include "stdio.h"

SceneData scene = {0};
unsigned int scene_ssbo;

HittableObject create_sphere(Vector3 center, float radius, Vector3 color) {
  HittableObject obj = {0};
  obj.type = SPHERE;
  obj.data1 = (Vector4){center.x, center.y, center.z, radius};
  obj.data2 = (Vector4){color.x, color.y, color.z, 0.0f};
  obj.data3 = (Vector4){0.0f, 0.0f, 0.0f, 0.0f}; // Material properties
  return obj;
}

void scene_add_object(HittableObject object, SceneData *scene) {
  if (scene->object_count > (sizeof(scene->objects) / sizeof(scene->objects[0]))) {
    syslog(LOG_ERR, "Too many objects... Increase MAX_OBJECTS");
    return;
  }
  scene->objects[scene->object_count] = object;
  scene->object_count++;
}

void initialise_scene() {
  scene_add_object(create_sphere((Vector3){0.0f, 0.0f, -1.0f}, 0.5f,
                                 (Vector3){1.0f, 0.2f, 0.2f}), &scene);
  scene_add_object(create_sphere((Vector3){0.0f, -100.5f, -1.0f}, 100.0f,
                                 (Vector3){1.0f, 1.0f, 0.2f}), &scene);

  // Create and upload SSBO
  scene_ssbo = rlLoadShaderBuffer(sizeof(SceneData), &scene, RL_DYNAMIC_COPY);
  printf("Scene SSBO created with ID: %u\n", scene_ssbo);
}

void update_scene() {
  // Update scene data when objects change
  rlUpdateShaderBuffer(scene_ssbo, &scene, sizeof(SceneData), 0);
}

int main() {
  // Screen
  /* float aspect_ratio = 16.0f / 9.0f; */
  /* int screen_width = 1000; */

  /* // Calculate screen height, ensure at least 1 */
  /* int screen_height = (int)(screen_width / aspect_ratio); */
  /* screen_height = (screen_height < 1) ? 1 : screen_height; */
  const int screen_width = 800;
  const int screen_height = 600;

  InitWindow(screen_width, screen_height, "Raytracer");

  camera_t camera = {0};
  initialise_camera(screen_width, screen_height, &camera);

  initialise_scene();


  // Load fragment shader
  Shader color_shader =
      LoadShader("shaders/vertex.glsl", "shaders/fragment.glsl");

  // Load compute shader
  int compute_shader = 0;
  unsigned int compute_texture = 0;
  char *shaderCode = LoadFileText("shaders/compute.glsl");

  int loc_time, loc_camera_center, loc_viewport_u, loc_viewport_v, loc_pixel00_loc, loc_pixel_delta_u, loc_pixel_delta_v, loc_screen_width, loc_screen_height;
  if (shaderCode) {
    int shaderData = rlCompileShader(shaderCode, RL_COMPUTE_SHADER);
    compute_shader = rlLoadComputeShaderProgram(shaderData);
    UnloadFileText(shaderCode);

    Image blankImage = GenImageColor(screen_width, screen_height, BLANK);
    compute_texture = rlLoadTexture(blankImage.data, screen_width, screen_height, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    UnloadImage(blankImage);

    // Get locations of uniforms
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

  Mesh quad = GenMeshPlane(2.0f, 2.0f, 1, 1);
  SetTargetFPS(0);

  while (!WindowShouldClose()) {
    if (compute_shader != 0) {

      rlEnableShader(compute_shader);

      // Bind ssbos
      rlBindShaderBuffer(scene_ssbo, 0);

      // Set uniforms
      rlSetUniform(loc_camera_center, &camera.camera_center.x, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_viewport_u, &camera.viewport_u.x, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_viewport_v, &camera.viewport_v.x, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_pixel00_loc, &camera.pixel00_loc.x, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_pixel_delta_u, &camera.pixel_delta_u.x, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_pixel_delta_v, &camera.pixel_delta_v.x, SHADER_UNIFORM_VEC3, 1);
      rlSetUniform(loc_screen_width, &screen_width, SHADER_UNIFORM_INT, 1);
      rlSetUniform(loc_screen_height, &screen_height, SHADER_UNIFORM_INT, 1);
      float t = GetTime();
      rlSetUniform(loc_time, &t, SHADER_UNIFORM_FLOAT, 1);

      rlBindImageTexture(compute_texture, 0, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);
      rlComputeShaderDispatch((screen_width + 7) / 8, (screen_height + 7) / 8, 1);
      rlDisableShader();
    }

    // Draw
    BeginDrawing();
    ClearBackground(BLACK);

    if (compute_shader != 0) {
      Rectangle source = {0, 0, screen_width, -screen_height};
      Rectangle dest = {0, 0, screen_width, screen_height};
      DrawTexturePro((Texture2D){
                         compute_texture, screen_width, screen_height, 1,
                         RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8},
                     source, dest, (Vector2){0, 0}, 0, WHITE);
    }

    DrawFPS(10, 10);
    EndDrawing();
  }

  UnloadShader(color_shader);
  UnloadMesh(quad);
  rlUnloadShaderProgram(compute_shader);
  rlUnloadTexture(compute_texture);
  CloseWindow();
  return 0;
}
