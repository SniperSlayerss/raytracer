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

// ==================== Definitions ====================
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

// ==================== Helper Functions  ====================
float random_float(in vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 random_vec3(in vec2 st, float min_val, float max_val) {
    vec3 rand = vec3(
        random_float(st),
        random_float(st + vec2(127.1, 311.7)),
        random_float(st + vec2(269.5, 183.3))
    );
    return min_val + rand * (max_val - min_val);
}

// vec3 random_unit_vector(in vec2 st) {
//     while (true) {
//     }
// }

// ==================== Interval ====================
float interval_size(Interval interval) { return interval.max - interval.min; }

bool interval_contains(Interval interval, float x) { return interval.min <= x && x <= interval.max; }

bool interval_surrounds(Interval interval, float x) { return interval.min < x && x < interval.max; }

// ==================== RAY ====================
vec3 ray_at_t(vec3 ray_origin, vec3 ray_dir, float t) { return ray_origin + t * ray_dir; }

void get_surface_normal(vec3 ray_dir, vec3 outward_normal, out vec3 normal, out bool is_front_face)
{
    is_front_face = dot(ray_dir, outward_normal) < 0.0;
    if(is_front_face) {
        normal = outward_normal;
    } else {
        normal = -outward_normal;
    }
}

bool hit_sphere(vec3 center, float radius, vec3 ray_origin, vec3 ray_dir, Interval ray_t, out HitRecord rec)
{
    vec3  oc = center - ray_origin;
    float a  = dot(ray_dir, ray_dir);
    float h  = dot(ray_dir, oc);
    float c  = dot(oc, oc) - radius * radius;

    float discriminant = h * h - a * c;
    if(discriminant < 0) {
        return false;
    }

    float sqrtd = sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range
    float root = (h - sqrtd) / a;
    if(!interval_surrounds(ray_t, root)) {
        root = (h + sqrtd) / a;
        if(!interval_surrounds(ray_t, root)) {
            return false;
        }
    }

    float t              = root;
    vec3  p              = ray_at_t(ray_origin, ray_dir, t);
    vec3  outward_normal = (p - center) / radius;

    vec3 surface_normal;
    bool is_front_face;
    get_surface_normal(ray_dir, outward_normal, surface_normal, is_front_face);

    rec.p      = p;
    rec.normal = surface_normal;
    rec.t      = t;
    return true;
}

bool hit_object(HittableObject obj, vec3 ray_origin, vec3 ray_dir, Interval ray_t, out HitRecord rec)
{
    if(obj.type == SPHERE) {
        return hit_sphere(obj.data1.xyz, obj.data1.w, ray_origin, ray_dir, ray_t, rec);
    }
    return false;
}

// Returns color
vec3 ray_trace(vec3 ray_origin, vec3 ray_dir, Interval ray_t, out HitRecord rec, out bool did_hit)
{
    HitRecord temp_rec = HitRecord(vec3(0,0,0), vec3(0,0,0), -1.0);
    HitRecord closest_rec = HitRecord(vec3(0.0), vec3(0.0), -1.0);
    int       hit_index   = -1;

    for (int i = 0; i < scene.object_count; i++) {
	float max_t = (closest_rec.t < 0.0) ? ray_t.max : closest_rec.t;
        if (hit_object(scene.objects[i], ray_origin, ray_dir,
                       Interval(ray_t.min, max_t), temp_rec)) {
            if (closest_rec.t < 0.0 || temp_rec.t < closest_rec.t) {
                closest_rec = temp_rec;
                hit_index = i;
            }
        }
    }

    did_hit = (hit_index != -1);
    rec     = closest_rec;
    return scene.objects[hit_index].data2.xyz;
}

vec3 ray_color(vec3 ray_origin, vec3 ray_dir)
{
    HitRecord rec = HitRecord(vec3(0,0,0), vec3(0,0,0), -1.0);
    bool      did_hit = false;
    vec3 color = ray_trace(ray_origin, ray_dir, Interval(0, 1.0 / 0.0), rec, did_hit);

    if(did_hit) {
        vec3 hit_point = ray_origin + rec.t * ray_dir;
        // return color;
        return 0.5 * vec3(rec.normal.x + 1, rec.normal.y + 1, rec.normal.z + 1);
    } else {
      	// Sky background
	float a = 0.5 * (ray_dir.y + 1.0);
	return (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
    }
}

// ========================================
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

    // Generate random numbers for x and y jitter, varying per pixel per frame
    // Jitter between -0.5 and 0.5 pixel unit
    vec3 offset = random_vec3(gl_GlobalInvocationID.xy + time, -0.5, 0.5);

    vec3 pixel_sample_loc = pixel00_loc + (x + offset.x) * pixel_delta_u + (y + offset.y) * pixel_delta_v;
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
