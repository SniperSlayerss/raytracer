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

void ray_trace(vec3 ray_origin, vec3 ray_dir, Interval ray_t, out HitRecord rec, out bool did_hit)
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
}

vec3 ray_color(vec3 ray_origin, vec3 ray_dir)
{
    HitRecord rec = HitRecord(vec3(0,0,0), vec3(0,0,0), -1.0);
    bool      did_hit = false;
    ray_trace(ray_origin, ray_dir, Interval(0, 1.0 / 0.0), rec, did_hit);

    if(did_hit) {
        vec3 hit_point = ray_origin + rec.t * ray_dir;
        return 0.5 * vec3(rec.normal.x + 1, rec.normal.y + 1, rec.normal.z + 1);
    } else {
      	// Sky background
	float a = 0.5 * (ray_dir.y + 1.0);
	return (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
    }
}