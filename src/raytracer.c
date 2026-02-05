#define GL_GLEXT_PROTOTYPES

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <GL/gl.h>

#include "camera.h"
#include "hittable.h"
#include "shaders.h"

#include <stdio.h>

const int screen_width  = 800;
const int screen_height = 600;

void initialise_scene(SceneData* scene)
{
    scene_add_object(create_sphere((Vector3) { 0.0f, 0.0f, -1.0f }, 0.5f, (Vector3) { 1.0f, 0.2f, 0.2f }), scene);
    scene_add_object(create_sphere((Vector3) { 0.0f, -100.5f, -1.0f }, 100.0f, (Vector3) { 1.0f, 1.0f, 0.2f }), scene);
}

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | RL_OPENGL_43); // Request OpenGL 4.3 context
    // Initialise
    InitWindow(screen_width, screen_height, "Raytracer");

    camera_t camera = { 0 };
    initialise_camera(screen_width, screen_height, &camera);

    SceneData scene = { 0 };
    initialise_scene(&scene);

    // Load compute shader
    ComputeShader compute = { 0 };
    shader_init_and_load(&compute, &scene, screen_width, screen_height);
    shader_bind_textures(&compute);

    bool reset_accumulation = false;

    SetTargetFPS(60);
    while(!WindowShouldClose()) {
        rlEnableShader(compute.id);

        shader_set_ssbos(&compute);
        shader_set_uniforms(&compute, &camera, screen_width, screen_height, GetTime(), reset_accumulation);

        rlComputeShaderDispatch((screen_width + 7) / 8, (screen_height + 7) / 8, 1);
        rlDisableShader();

        glTextureBarrier();

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle source = { 0, 0, screen_width, -screen_height };
        Rectangle dest   = { 0, 0, screen_width, screen_height };
        DrawTexturePro((Texture2D) { compute.Textures.compute_texture, screen_width, screen_height, 1, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 },
                       source,
                       dest,
                       (Vector2) { 0, 0 },
                       0,
                       WHITE);

        DrawFPS(10, 10);
        EndDrawing();
    }

    // TODO: Make function in shaders.c
    rlUnloadShaderProgram(compute.id);
    rlUnloadTexture(compute.Textures.compute_texture);
    glDeleteTextures(1, &compute.Textures.accumulated_color_texture);
    glDeleteTextures(1, &compute.Textures.sample_count_texture);
    rlUnloadShaderBuffer(compute.SSBOs.scene_ssbo);
    CloseWindow();
    return 0;
}
