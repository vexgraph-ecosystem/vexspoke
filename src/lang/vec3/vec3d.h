#ifndef LANG_VEC3_VEC3D_H
#define LANG_VEC3_VEC3D_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "math/coord_frame.h"

// lang/vec3/vec3d.h — 32-byte SIMD Double-Precision 3D Spatial Vector.
//
// Single Class Per File Law: Vec3d.
//
// Ultra-precision 3D vector for celestial physics, gravitational simulations,
// and orbital integration. Aligned to 32 bytes for AVX2 / Apple NEON pair.

typedef struct Vec3d {
    alignas(32) union {
        struct { double horizontal; double vertical; double depth; uint64_t frame; };
        struct { double x; double y; double z; uint64_t _frame; };
        struct { double right; double up; double front; uint64_t _f; };
        double data[4];
    };
} Vec3d;

#define VEC3D_BYTES 32u

Vec3d *Vec3d_0(void);
Vec3d *Vec3d_3(double horizontal, double vertical, double depth);
Vec3d *Vec3d_4(double horizontal, double vertical, double depth, CoordFrame frame);
void Vec3d_free(Vec3d *v);

// Spatial Directional Getters & Setters
double Vec3d_getRight(const Vec3d *v);
void   Vec3d_setRight(Vec3d *v, double val);
double Vec3d_getLeft(const Vec3d *v);
void   Vec3d_setLeft(Vec3d *v, double val);

double Vec3d_getUp(const Vec3d *v);
void   Vec3d_setUp(Vec3d *v, double val);
double Vec3d_getDown(const Vec3d *v);
void   Vec3d_setDown(Vec3d *v, double val);

double Vec3d_getFront(const Vec3d *v);
void   Vec3d_setFront(Vec3d *v, double val);
double Vec3d_getBack(const Vec3d *v);
void   Vec3d_setBack(Vec3d *v, double val);

// Frame-mapped getters
double Vec3d_getX(const Vec3d *v);
double Vec3d_getY(const Vec3d *v);
double Vec3d_getZ(const Vec3d *v);

void Vec3d_add(const Vec3d *a, const Vec3d *b, Vec3d *dest);
void Vec3d_sub(const Vec3d *a, const Vec3d *b, Vec3d *dest);
void Vec3d_mul(const Vec3d *a, double scalar, Vec3d *dest);
double Vec3d_dot(const Vec3d *a, const Vec3d *b);
double Vec3d_length(const Vec3d *v);
void Vec3d_normalize(const Vec3d *src, Vec3d *dest);

#endif
