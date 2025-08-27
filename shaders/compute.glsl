#version 430

// Compute shader work group size
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Output image
layout(rgba8, binding = 0) uniform image2D imgOutput;

uniform vec3 camera_center;
uniform vec3 viewport_u;
uniform vec3 viewport_v;
uniform vec3 pixel00_loc;
uniform vec3 pixel_delta_u;
uniform vec3 pixel_delta_v;

uniform float time;

uniform int screen_width;
uniform int screen_height;

bool hit_sphere(vec3 center, float radius, vec3 ray_origin, vec3 ray_dir) {
    vec3 oc = center - ray_origin;
    float a = dot(ray_dir, ray_dir);
    float b = -2.0 * dot(ray_dir, oc);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4.0 * a * c;
    return discriminant >= 0.0;
}

vec3 calculate_color(int x, int y, vec3 ray_direction) {
    float a = 0.5 * (ray_direction.y + 1.0);
    vec3 color = (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
    return color;
}

void main() {
    // Get current pixel coordinates
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(imgOutput);

    int x = pixel.x;
    int y = screen_height - 1 - pixel.y;
    
    // Check bounds
    if (pixel.x >= size.x || pixel.y >= size.y) return;

    vec3 pixel_center = pixel00_loc + x * pixel_delta_u + y * pixel_delta_v;
    vec3 ray_direction = normalize(pixel_center - camera_center);

    vec3 color;
    if (hit_sphere(vec3(0.0, 0.0, -1.0), 0.5, camera_center, ray_direction)) {
        color = vec3(1, 0, 0); 
    } else {
	color = calculate_color(x, y, ray_direction);
    }
    
    // Write the color to the output image
    imageStore(imgOutput, pixel, vec4(color, 1.0));
}