#version 430

#define SPHERE 0
#define MAX_OBJECTS 100

// Compute shader work group size
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Output image
layout(rgba8, binding = 0) uniform image2D imgOutput;

// Scene data structure
struct HittableObject {
    int type;
    float padding[3];
    vec4 data1;
    vec4 data2;
    vec4 data3;
};

struct SceneData {
    HittableObject objects[MAX_OBJECTS];
    int object_count;
    float padding[3];
};

// SSBO for scene data
layout(std430, binding = 0) buffer SceneBuffer {
    SceneData scene;
};

uniform vec3 camera_center;
uniform vec3 viewport_u;
uniform vec3 viewport_v;
uniform vec3 pixel00_loc;
uniform vec3 pixel_delta_u;
uniform vec3 pixel_delta_v;

uniform float time;

uniform int screen_width;
uniform int screen_height;

vec3 ray_at_t(vec3 ray_origin, vec3 ray_dir, float t) {
    return ray_origin + t * ray_dir;
}

float hit_sphere(vec3 center, float radius, vec3 ray_origin, vec3 ray_dir) {
    vec3 oc = center - ray_origin;
    float a = dot(ray_dir, ray_dir);
    float h = dot(ray_dir, oc);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = h * h - a * c;

    if (discriminant < 0) {
       return -1.0;
    } else {
       return (h - sqrt(discriminant)) / a;
    }
}

float hit_object(HittableObject obj, vec3 ray_origin, vec3 ray_dir) {
    if (obj.type == SPHERE) {
        return hit_sphere(obj.data1.xyz, obj.data1.w, ray_origin, ray_dir);
    }
    return -1.0;
}


vec3 get_outward_normal(HittableObject obj, vec3 hit_point) {
    if (obj.type == SPHERE) {
        return normalize(hit_point - obj.data1.xyz) / obj.data1.w;
    }
    return vec3(0.0, 1.0, 0.0);
}

void get_surface_normal(HittableObject obj, vec3 ray_dir, vec3 outward_normal, out vec3 normal, out bool is_front_face) {
    if (obj.type == SPHERE) {
	is_front_face = dot(ray_dir, outward_normal) < 0.0;
	if (is_front_face) {
	    normal = outward_normal; 
	} else {
	    normal = -outward_normal; 
	}
    }
}

vec3 get_color(HittableObject obj) {
    if (obj.type == SPHERE) {
        return obj.data2.xyz;
    }
    return vec3(1.0, 0.0, 1.0); // Magenta for unknown
}
vec3 calculate_background(vec3 ray_dir) {
    float a = 0.5 * (ray_dir.y + 1.0);
    return (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
}

vec3 trace_ray(vec3 ray_origin, vec3 ray_dir) {
    float closest_t = -1.0;
    int hit_index = -1;
    
    // Find closest intersection
    for (int i = 0; i < scene.object_count; i++) {
        float t = hit_object(scene.objects[i], ray_origin, ray_dir);
        if (t > 0.0 && (closest_t < 0.0 || t < closest_t)) {
            closest_t = t;
            hit_index = i;
        }
    }
    
    if (hit_index >= 0) {
        // Calculate hit point and normal
        vec3 hit_point = ray_origin + closest_t * ray_dir;
        vec3 outward_normal = get_outward_normal(scene.objects[hit_index], hit_point);
	vec3 surface_normal;
	bool is_front_face;
	get_surface_normal(scene.objects[hit_index], ray_dir, outward_normal, surface_normal, is_front_face);

        vec3 color = get_color(scene.objects[hit_index]);
        
        return 0.5 * vec3(surface_normal.x + 1, surface_normal.y + 1, surface_normal.z + 1);
        // return color;
    } else {
        return calculate_background(ray_dir);
    }
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
    vec3 ray_dir = normalize(pixel_center - camera_center);

    vec3 color = trace_ray(camera_center, ray_dir);
    
    // Write the color to the output image
    imageStore(imgOutput, pixel, vec4(color, 1.0));
}