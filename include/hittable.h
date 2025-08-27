#ifndef HITTABLE_H
#define HITTABLE_H

#define SPHERE 0

#define MAX_OBJECTS 100

typedef struct {
  int type;
  float padding[3]; // GPU alignment
  Vector4 data1;
  Vector4 data2;
  Vector4 data3;
} HittableObject;

typedef struct {
    HittableObject objects[MAX_OBJECTS];
    int object_count;
    float padding[3];
}SceneData;

#endif
