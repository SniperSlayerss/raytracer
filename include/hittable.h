#ifndef HITTABLE_H
#define HITTABLE_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define SPHERE 0

#define MAX_OBJECTS 100

typedef struct {
    int     type;
    float   padding[3]; // GPU alignment
    Vector4 data1;
    Vector4 data2;
    Vector4 data3;
} HittableObject;

typedef struct {
    HittableObject objects[MAX_OBJECTS];
    int            object_count;
    float          padding[3];
} SceneData;

void scene_add_object(HittableObject object, SceneData* scene);
void update_scene(SceneData* scene, unsigned int* scene_ssbo);

HittableObject create_sphere(Vector3 center, float radius, Vector3 color);

#endif // HITTABLE_H
