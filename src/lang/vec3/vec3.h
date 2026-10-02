#ifndef LANG_VEC3_VEC3_H
#define LANG_VEC3_VEC3_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "c23/constructor.h"
#include "math/coord_frame.h"

// lang/vec3/vec3.h — 16-byte SIMD Spatial 3D Vector.
//
// Single Class Per File Law: Vec3.
//
// Represents a coordinate-agnostic 3D spatial vector with horizontal, vertical,
// and depth dimensions, carrying its CoordFrame as true embedded metadata in
// the 4th float slot. Preserves 16-byte SIMD alignment for NEON / SSE.
//
// Spatial semantic axes:
//   - horizontal (x): Right (+), Left (-)
//   - vertical   (y): Up (+),    Down (-)
//   - depth      (z): Front (+), Back (-)
//
// Direct directional inverses:
//   getBack() == -getFront() (e.g. if front is 28, back is -28)
//   getLeft() == -getRight()
//   getDown() == -getUp()

typedef struct Vec3 {
    alignas(16) union {
        struct { float horizontal; float vertical; float depth; uint32_t frame; };
        struct { float x; float y; float z; uint32_t _frame; };
        struct { float right; float up; float front; uint32_t _f; };
        float data[4];
    };
} Vec3;

#define VEC3_BYTES 16u

// Constructors
Vec3 *Vec3_0(void);
Vec3 *Vec3_3(float horizontal, float vertical, float depth);
Vec3 *Vec3_4(float horizontal, float vertical, float depth, CoordFrame frame);
void Vec3_free(Vec3 *v);

// Spatial Directional Getters & Setters
float Vec3_getRight(const Vec3 *v);
void Vec3_setRight(Vec3 *v, float val);
float Vec3_getLeft(const Vec3 *v);
void Vec3_setLeft(Vec3 *v, float val);

float Vec3_getUp(const Vec3 *v);
void Vec3_setUp(Vec3 *v, float val);
float Vec3_getDown(const Vec3 *v);
void Vec3_setDown(Vec3 *v, float val);

float Vec3_getFront(const Vec3 *v);
void Vec3_setFront(Vec3 *v, float val);
float Vec3_getBack(const Vec3 *v);
void Vec3_setBack(Vec3 *v, float val);

// Frame Metadata Accessors
CoordFrame Vec3_getFrame(const Vec3 *v);
void Vec3_setFrame(Vec3 *v, CoordFrame frame);

// Frame-mapped One-Line Coordinate Accessors
float Vec3_getX(const Vec3 *v);
void Vec3_setX(Vec3 *v, float x);
float Vec3_getY(const Vec3 *v);
void Vec3_setY(Vec3 *v, float y);
float Vec3_getZ(const Vec3 *v);
void Vec3_setZ(Vec3 *v, float z);

float Vec3_getXInFrame(const Vec3 *v, CoordFrame frame);
float Vec3_getYInFrame(const Vec3 *v, CoordFrame frame);
float Vec3_getZInFrame(const Vec3 *v, CoordFrame frame);

void Vec3_set(Vec3 *v, float horizontal, float vertical, float depth);
void Vec3_copy(const Vec3 *src, Vec3 *dest);

// Coordinate Frame Transform
void Vec3_toFrame(const Vec3 *src, CoordFrame targetFrame, Vec3 *dest);

// Vector Arithmetic
void Vec3_add(const Vec3 *a, const Vec3 *b, Vec3 *dest);
void Vec3_sub(const Vec3 *a, const Vec3 *b, Vec3 *dest);
void Vec3_mul(const Vec3 *a, float scalar, Vec3 *dest);
void Vec3_div(const Vec3 *a, float scalar, Vec3 *dest);

float Vec3_dot(const Vec3 *a, const Vec3 *b);
void Vec3_cross(const Vec3 *a, const Vec3 *b, Vec3 *dest);

float Vec3_lengthSquared(const Vec3 *v);
float Vec3_length(const Vec3 *v);

// Normalization: Strict IEEE (1/sqrt) vs Fast Relaxed (Quake invSqrt)
void Vec3_normalize(const Vec3 *src, Vec3 *dest);
void Vec3_fastNormalize(const Vec3 *src, Vec3 *dest);

float Vec3_distance(const Vec3 *a, const Vec3 *b);
float Vec3_angle(const Vec3 *a, const Vec3 *b);

void Vec3_project(const Vec3 *vector, const Vec3 *onto, Vec3 *dest);
void Vec3_reflect(const Vec3 *incident, const Vec3 *normal, Vec3 *dest);
void Vec3_clamp(const Vec3 *src, float min_val, float max_val, Vec3 *dest);
void Vec3_abs(const Vec3 *src, Vec3 *dest);
void Vec3_lerp(const Vec3 *a, const Vec3 *b, float t, Vec3 *dest);

#endif
