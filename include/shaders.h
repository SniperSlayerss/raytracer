#ifndef SHADERS_H
#define SHADERS_H

#include "raylib.h"
#include "rlgl.h"

#include "camera.h"
#include "hittable.h"

// For shaders/compute.glsl
typedef struct {
    int   id;
    char* name;
    bool  is_loaded;

    struct {
	unsigned int compute_texture;
        unsigned int accumulated_color_texture;
	unsigned int sample_count_texture;
    } Textures;

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
	int loc_reset_accumulation;
    } Uniforms;

    struct {
        unsigned int scene_ssbo;
    } SSBOs;
} ComputeShader;

bool shader_init_and_load(ComputeShader* compute, SceneData* scene, const int screen_width, const int screen_height);
void shader_set_uniforms(ComputeShader* compute, camera_t* camera, const int screen_width, const int screen_height, float time, bool reset_accumulation);
void shader_set_ssbos(ComputeShader* compute);
void shader_bind_textures(ComputeShader* compute);

#endif // SHADERS_H
