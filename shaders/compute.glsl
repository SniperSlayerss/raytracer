#version 430

#define SPHERE 0

#define MAX_OBJECTS 100

// Compute shader work group size
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Output image
layout(rgba8, binding = 0) uniform image2D imgOutput;

// Accumulated color buffer
layout(rgba32f, binding = 1) uniform image2D imgAccumulatedColor;
// Sample count buffer
layout(r32ui, binding = 2) uniform uimage2D imgSampleCount;

struct Interval {
    float min;
    float max;
};

struct HitRecord {
    vec3  p;
    vec3  normal;
    float t;
};

struct HittableObject {
    int   type;
    float padding[3];
    vec4  data1;
    vec4  data2;
    vec4  data3;
};

struct SceneData {
    HittableObject objects[MAX_OBJECTS];
    int            object_count;
    float          padding[3];
};

// SSBO for scene data
layout(std430, binding = 0) buffer SceneBuffer { SceneData scene; };

uniform vec3 camera_center;
uniform vec3 viewport_u;
uniform vec3 viewport_v;
uniform vec3 pixel00_loc;
uniform vec3 pixel_delta_u;
uniform vec3 pixel_delta_v;

uniform float time;

uniform int screen_width;
uniform int screen_height;

// When to reset accumulation
uniform int reset_accumulation;

float random(in vec2 _st) { return fract(sin(dot(_st.xy, vec2(12.9898, 78.233))) * 43758.5453123); }

#include "interval.glsl"
#include "ray.glsl"

void main()
{
    // Get current pixel coordinates
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(imgOutput);

    // Make top left 0,0
    int x = pixel.x;
    int y = screen_height - 1 - pixel.y;

    // Check bounds
    if(pixel.x >= size.x || pixel.y >= size.y)
        return;

    // Generate two independent random numbers for x and y jitter, varying per pixel per frame
    float x_offset = random(vec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y) * 1.337 + time) - 0.5; // Jitter between -0.5 and 0.5 pixel unit
    float y_offset = random(vec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y) * 2.718 + time + 100.0) - 0.5; // Use a different seed for y-jitter

    vec3 pixel_sample_loc = pixel00_loc + (x + x_offset) * pixel_delta_u + (y + y_offset) * pixel_delta_v;
    vec3 ray_dir      = normalize(pixel_sample_loc - camera_center);

    vec3 current_frame_color = ray_color(camera_center, ray_dir);

    // Read accumulated average data for current pixel
    vec4 accumulated_color_prev = imageLoad(imgAccumulatedColor, pixel);
    uint sample_count_prev = imageLoad(imgSampleCount, pixel).x;

    // Clear accumulated info if reset is triggered
    if (reset_accumulation != 0) {
        accumulated_color_prev = vec4(0.0);
        sample_count_prev = 0u;
    }

    // Accumulated average pixel color
    vec4 accumulated_color_next = accumulated_color_prev + vec4(current_frame_color, 1.0f);
    uint sample_count_next = sample_count_prev + 1u;

    // Store accumulated buffers
    imageStore(imgAccumulatedColor, pixel, accumulated_color_next);
    imageStore(imgSampleCount, pixel, uvec4(sample_count_next)); // Write to .x component

    vec3 final_color = accumulated_color_next.rgb / float(sample_count_next);
    final_color = clamp(final_color, 0.0, 1.0);
    // Write the color to the output image
    imageStore(imgOutput, pixel, vec4(final_color, 1.0));
}