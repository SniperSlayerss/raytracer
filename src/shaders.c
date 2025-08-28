#include "shaders.h"

bool shader_init_and_load(ComputeShader* compute, SceneData* scene)
{
    compute->name      = "shaders/compute.glsl";
    compute->id        = 0;
    compute->is_loaded = false;

    // Load compute shader
    char* shaderCode = LoadShaderWithIncludes(compute->name);
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

    // Get locations of uniforms
    compute->Uniforms.loc_camera_center = rlGetLocationUniform(compute->id, "camera_center");
    compute->Uniforms.loc_viewport_u    = rlGetLocationUniform(compute->id, "viewport_u");
    compute->Uniforms.loc_viewport_v    = rlGetLocationUniform(compute->id, "viewport_v");
    compute->Uniforms.loc_pixel00_loc   = rlGetLocationUniform(compute->id, "pixel00_loc");
    compute->Uniforms.loc_pixel_delta_u = rlGetLocationUniform(compute->id, "pixel_delta_u");
    compute->Uniforms.loc_pixel_delta_v = rlGetLocationUniform(compute->id, "pixel_delta_v");
    compute->Uniforms.loc_time          = rlGetLocationUniform(compute->id, "time");
    compute->Uniforms.loc_screen_width  = rlGetLocationUniform(compute->id, "screen_width");
    compute->Uniforms.loc_screen_height = rlGetLocationUniform(compute->id, "screen_height");

    // Create and upload SSBO
    compute->SSBOs.scene_ssbo = rlLoadShaderBuffer(sizeof(SceneData), scene, RL_DYNAMIC_COPY);
    printf("Scene SSBO created with ID: %u\n", compute->SSBOs.scene_ssbo);

    return true;
}

void shader_set_uniforms(ComputeShader* compute, camera_t* camera, int screen_width, int screen_height)
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
    float t = GetTime();
    rlSetUniform(compute->Uniforms.loc_time, &t, SHADER_UNIFORM_FLOAT, 1);
}

void shader_set_ssbos(ComputeShader* compute) { rlBindShaderBuffer(compute->SSBOs.scene_ssbo, 0); }
