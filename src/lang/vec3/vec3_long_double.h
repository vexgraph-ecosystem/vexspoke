#ifndef LANG_VEC3_VEC3_LONG_DOUBLE_H
#define LANG_VEC3_VEC3_LONG_DOUBLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "c23/constructor.h"

#include "math/coord_frame.h"

// lang/vec3/vec3_long_double.h — Cosmic-Scale 3D Vector (int64 sector + double local).
//
// Single Class Per File Law: Vec3LongDouble.
//
// Unbounded spatial coordinate representation capable of addressing solar
// systems down to sub-millimeter precision without loss of significance.

typedef struct CoordLongDouble {
    int64_t sector;
    double local;
} CoordLongDouble;

typedef struct Vec3LongDouble {
    CoordLongDouble horizontal;
    CoordLongDouble vertical;
    CoordLongDouble depth;
    uint64_t frame;
} Vec3LongDouble;

Vec3LongDouble *Vec3LongDouble_0(void);
Vec3LongDouble *Vec3LongDouble_7(int64_t sectorH, double localH, int64_t sectorV, double localV,
                                 int64_t sectorD, double localD, CoordFrame frame);
#define Vec3LongDouble(...) CONSTRUCTOR_DISPATCH(Vec3LongDouble, __VA_ARGS__)
void Vec3LongDouble_free(Vec3LongDouble *v);

void Vec3LongDouble_rebalance(Vec3LongDouble *v, double sectorSize);

double Vec3LongDouble_getRightLocal(const Vec3LongDouble *v);
double Vec3LongDouble_getLeftLocal(const Vec3LongDouble *v);
double Vec3LongDouble_getUpLocal(const Vec3LongDouble *v);
double Vec3LongDouble_getDownLocal(const Vec3LongDouble *v);
double Vec3LongDouble_getFrontLocal(const Vec3LongDouble *v);
double Vec3LongDouble_getBackLocal(const Vec3LongDouble *v);

#endif
