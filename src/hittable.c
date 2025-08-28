#include "hittable.h"

HittableObject create_sphere(Vector3 center, float radius, Vector3 color)
{
    HittableObject obj;
    obj.type = SPHERE;
    obj.data1 = (Vector4) { center.x, center.y, center.z, radius };
    obj.data2 = (Vector4) { color.x, color.y, color.z, 0.0f };
    obj.data3 = (Vector4) { 0.0f, 0.0f, 0.0f, 0.0f };
    return obj;
}

void scene_add_object(HittableObject object, SceneData* scene)
{
    if (scene->object_count > (sizeof(scene->objects) / sizeof(scene->objects[0]))) {
        fprintf(stderr, "ERROR: Too many objects... Increase MAX_OBJECTS");
        return;
    }
    scene->objects[scene->object_count] = object;
    scene->object_count++;
}

void update_scene(SceneData* scene, unsigned int* scene_ssbo)
{
    // Update scene data when objects change
    rlUpdateShaderBuffer(*scene_ssbo, &scene, sizeof(SceneData), 0);
}
