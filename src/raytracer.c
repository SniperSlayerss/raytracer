#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

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
    // Initialise
    InitWindow(screen_width, screen_height, "Raytracer");

    camera_t camera = { 0 };
    initialise_camera(screen_width, screen_height, &camera);

    SceneData    scene = { 0 };
    initialise_scene(&scene);

    // Load compute shader
    ComputeShader compute = { 0 };
    shader_init_and_load(&compute, &scene);

    // Create texture to render compute shader on
    unsigned int compute_texture = 0;
    Image        blankImage      = GenImageColor(screen_width, screen_height, BLANK);
    compute_texture              = rlLoadTexture(blankImage.data, screen_width, screen_height, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    UnloadImage(blankImage);

    SetTargetFPS(0);
    while(!WindowShouldClose()) {
        rlEnableShader(compute.id);

	shader_set_ssbos(&compute);
	shader_set_uniforms(&compute, &camera, screen_width, screen_height);

        // Allow compute shader to write to texture
        rlBindImageTexture(compute_texture, 0, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);
        rlComputeShaderDispatch((screen_width + 7) / 8, (screen_height + 7) / 8, 1);
        rlDisableShader();

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle source = { 0, 0, screen_width, -screen_height };
        Rectangle dest   = { 0, 0, screen_width, screen_height };
        DrawTexturePro((Texture2D) { compute_texture, screen_width, screen_height, 1, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 },
                       source,
                       dest,
                       (Vector2) { 0, 0 },
                       0,
                       WHITE);

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlUnloadShaderProgram(compute.id);
    rlUnloadTexture(compute_texture);
    CloseWindow();
    return 0;
}
