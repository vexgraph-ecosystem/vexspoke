#ifndef LANG_VEC3_VEC3_INT_FLOAT_H
#define LANG_VEC3_VEC3_INT_FLOAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "math/coord_frame.h"

// lang/vec3/vec3_int_float.h — Large-World 3D Vector (int32 sector + float local).
//
// Single Class Per File Law: Vec3IntFloat.
//
// Eliminates 32-bit floating-point jitter at large distances by pairing
// an integer grid tile / sector with a normalized local offset per axis.

typedef struct CoordIntFloat {
    int32_t sector;
    float local;
} CoordIntFloat;

typedef struct Vec3IntFloat {
    CoordIntFloat horizontal;
    CoordIntFloat vertical;
    CoordIntFloat depth;
    uint32_t frame;
} Vec3IntFloat;

Vec3IntFloat *Vec3IntFloat_0(void);
Vec3IntFloat *Vec3IntFloat_create(int32_t sH, float lH, int32_t sV, float lV, int32_t sD, float lD, CoordFrame frame);
void Vec3IntFloat_free(Vec3IntFloat *v);

// Normalize sector overflow: when |local| >= sectorSize, bump sector
void Vec3IntFloat_rebalance(Vec3IntFloat *v, float sectorSize);

// Directional queries
float Vec3IntFloat_getRightLocal(const Vec3IntFloat *v);
float Vec3IntFloat_getLeftLocal(const Vec3IntFloat *v);
float Vec3IntFloat_getUpLocal(const Vec3IntFloat *v);
float Vec3IntFloat_getDownLocal(const Vec3IntFloat *v);
float Vec3IntFloat_getFrontLocal(const Vec3IntFloat *v);
float Vec3IntFloat_getBackLocal(const Vec3IntFloat *v);

#endif
