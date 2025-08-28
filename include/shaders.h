#ifndef SHADERS_H
#define SHADERS_H

#include "raylib.h"

#include "camera.h"
#include "hittable.h"
#include "shader_parser.h"

// For shaders/compute.glsl
typedef struct {
    int   id;
    char* name;
    bool  is_loaded;

    struct {
        int loc_camera_center;
        int loc_viewport_u;
        int loc_viewport_v;
        int loc_pixel00_loc;
        int loc_pixel_delta_u;
        int loc_pixel_delta_v;
        int loc_time;
        int loc_screen_width;
        int loc_screen_height;
    } Uniforms;

    struct {
        unsigned int scene_ssbo;
    } SSBOs;
} ComputeShader;

bool shader_init_and_load(ComputeShader* compute, SceneData* scene);
void shader_set_uniforms(ComputeShader* compute, camera_t* camera, int screen_width, int screen_height);
void shader_set_ssbos(ComputeShader* compute);

#endif // SHADERS_H
