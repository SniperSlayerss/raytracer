#define GL_GLEXT_PROTOTYPES

#include "shaders.h"
#include <GL/gl.h>

bool shader_init_and_load(ComputeShader* compute, SceneData* scene, const int screen_width, const int screen_height)
{
    compute->name      = "shaders/compute.glsl";
    compute->id        = 0;
    compute->is_loaded = false;

    // Load compute shader
    char* shaderCode = LoadFileText(compute->name);
    if(!shaderCode) {
        fprintf(stderr, "ERROR: Failed to get compute shader\n");
        return false;
    }

    // Compile compute shader
    int shaderData = rlCompileShader(shaderCode, RL_COMPUTE_SHADER);
    if(!shaderData) {
        fprintf(stderr, "ERROR: Failed to compile compute shader\n");
        return false;
    }

    compute->id = rlLoadComputeShaderProgram(shaderData);
    if(!compute->id) {
        fprintf(stderr, "ERROR: Failed to load compute shader\n");
        return false;
    }
    compute->is_loaded = true;

    UnloadFileText(shaderCode);

    // Create texture to render compute shader on
    Image blankImage = GenImageColor(screen_width, screen_height, BLANK);
    compute->Textures.compute_texture  = rlLoadTexture(blankImage.data, screen_width, screen_height, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    UnloadImage(blankImage);

    // Get locations of uniforms
    compute->Uniforms.loc_camera_center      = rlGetLocationUniform(compute->id, "camera_center");
    compute->Uniforms.loc_viewport_u         = rlGetLocationUniform(compute->id, "viewport_u");
    compute->Uniforms.loc_viewport_v         = rlGetLocationUniform(compute->id, "viewport_v");
    compute->Uniforms.loc_pixel00_loc        = rlGetLocationUniform(compute->id, "pixel00_loc");
    compute->Uniforms.loc_pixel_delta_u      = rlGetLocationUniform(compute->id, "pixel_delta_u");
    compute->Uniforms.loc_pixel_delta_v      = rlGetLocationUniform(compute->id, "pixel_delta_v");
    compute->Uniforms.loc_time               = rlGetLocationUniform(compute->id, "time");
    compute->Uniforms.loc_screen_width       = rlGetLocationUniform(compute->id, "screen_width");
    compute->Uniforms.loc_screen_height      = rlGetLocationUniform(compute->id, "screen_height");
    compute->Uniforms.loc_reset_accumulation = rlGetLocationUniform(compute->id, "reset_accumulation");

    // Create and upload SSBO
    compute->SSBOs.scene_ssbo = rlLoadShaderBuffer(sizeof(SceneData), scene, RL_DYNAMIC_COPY);
    printf("Scene SSBO created with ID: %u\n", compute->SSBOs.scene_ssbo);

    // imgAccumulatedColor (binding 1): RGBA32F for high precision
    glGenTextures(1, &compute->Textures.accumulated_color_texture);
    glBindTexture(GL_TEXTURE_2D, compute->Textures.accumulated_color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // No filtering for image storage
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind

    // imgSampleCount (binding 2): R32UI for integer counting
    glGenTextures(1, &compute->Textures.sample_count_texture);
    glBindTexture(GL_TEXTURE_2D, compute->Textures.sample_count_texture);
    // Use GL_RED_INTEGER and GL_UNSIGNED_INT for R32UI format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, screen_width, screen_height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind

    return true;
}

void shader_set_uniforms(ComputeShader* compute, camera_t* camera, const int screen_width, const int screen_height, float time, bool reset_accumulation)
{
    // Set uniforms
    rlSetUniform(compute->Uniforms.loc_camera_center, &camera->camera_center.x, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(compute->Uniforms.loc_viewport_u, &camera->viewport_u.x, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(compute->Uniforms.loc_viewport_v, &camera->viewport_v.x, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(compute->Uniforms.loc_pixel00_loc, &camera->pixel00_loc.x, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(compute->Uniforms.loc_pixel_delta_u, &camera->pixel_delta_u.x, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(compute->Uniforms.loc_pixel_delta_v, &camera->pixel_delta_v.x, SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(compute->Uniforms.loc_screen_width, &screen_width, SHADER_UNIFORM_INT, 1);
    rlSetUniform(compute->Uniforms.loc_screen_height, &screen_height, SHADER_UNIFORM_INT, 1);
    rlSetUniform(compute->Uniforms.loc_time, &time, SHADER_UNIFORM_FLOAT, 1);

    // Weird bug when using rlSetUniform
    // rlSetUniform(compute->Uniforms.loc_reset_accumulation, &reset_accumulation, SHADER_UNIFORM_INT, 1);
    glUniform1i(compute->Uniforms.loc_reset_accumulation, (int)reset_accumulation); // Send 0 (false)
}

void shader_set_ssbos(ComputeShader* compute) { rlBindShaderBuffer(compute->SSBOs.scene_ssbo, 0); }

void shader_bind_textures(ComputeShader* compute)
{
    // Bind image textures for read/write access in the compute shader

    // Binding 0: Output texture (rgba8) - `imgOutput` in shader
    // `compute->outputTexture` must be set in main() after its creation
    rlBindImageTexture(compute->Textures.compute_texture, 0, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, false);

    // Binding 1: Accumulated Color (rgba32f) - `imgAccumulatedColor` in shader
    // RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32 maps to GL_RGBA32F
    rlBindImageTexture(compute->Textures.accumulated_color_texture, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, false);

    // Binding 2: Sample Count (r32ui) - `imgSampleCount` in shader
    // Raylib's `rlBindImageTexture` doesn't directly support `GL_R32UI` format via `RL_PIXELFORMAT`
    // We use raw `glBindImageTexture` here for precise control.
    glBindImageTexture(2, compute->Textures.sample_count_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}
