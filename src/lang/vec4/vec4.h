#ifndef LANG_VEC4_VEC4_H
#define LANG_VEC4_VEC4_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "c23/constructor.h"
#include "math/coord_frame.h"

// lang/vec4/vec4.h — 16-byte SIMD Spatial 4D / Homogeneous Vector.
//
// Single Class Per File Law: Vec4.
//
// Represents a 4D spatial / projective / quaternion vector with horizontal,
// vertical, depth, and w components. Preserves 16-byte SIMD alignment.

typedef struct Vec4 {
    alignas(16) union {
        struct { float horizontal; float vertical; float depth; float w; };
        struct { float x; float y; float z; float _w; };
        struct { float right; float up; float front; float _w2; };
        struct { float r; float g; float b; float a; };
        float data[4];
    };
} Vec4;

#define VEC4_BYTES 16u

Vec4 *Vec4_0(void);
Vec4 *Vec4_4(float horizontal, float vertical, float depth, float w);
void Vec4_free(Vec4 *v);

// Spatial Directional Getters & Setters (w is untouched)
float Vec4_getRight(const Vec4 *v);
void  Vec4_setRight(Vec4 *v, float val);
float Vec4_getLeft(const Vec4 *v);
void  Vec4_setLeft(Vec4 *v, float val);

float Vec4_getUp(const Vec4 *v);
void  Vec4_setUp(Vec4 *v, float val);
float Vec4_getDown(const Vec4 *v);
void  Vec4_setDown(Vec4 *v, float val);

float Vec4_getFront(const Vec4 *v);
void  Vec4_setFront(Vec4 *v, float val);
float Vec4_getBack(const Vec4 *v);
void  Vec4_setBack(Vec4 *v, float val);

float Vec4_getX(const Vec4 *v);
void  Vec4_setX(Vec4 *v, float x);
float Vec4_getY(const Vec4 *v);
void  Vec4_setY(Vec4 *v, float y);
float Vec4_getZ(const Vec4 *v);
void  Vec4_setZ(Vec4 *v, float z);
float Vec4_getW(const Vec4 *v);
void  Vec4_setW(Vec4 *v, float w);

float Vec4_getXInFrame(const Vec4 *v, CoordFrame frame);
float Vec4_getYInFrame(const Vec4 *v, CoordFrame frame);
float Vec4_getZInFrame(const Vec4 *v, CoordFrame frame);

void Vec4_set(Vec4 *v, float horizontal, float vertical, float depth, float w);
void Vec4_copy(const Vec4 *src, Vec4 *dest);

void Vec4_add(const Vec4 *a, const Vec4 *b, Vec4 *dest);
void Vec4_sub(const Vec4 *a, const Vec4 *b, Vec4 *dest);
void Vec4_mul(const Vec4 *a, float scalar, Vec4 *dest);
void Vec4_div(const Vec4 *a, float scalar, Vec4 *dest);

float Vec4_dot(const Vec4 *a, const Vec4 *b);
float Vec4_lengthSquared(const Vec4 *v);
float Vec4_length(const Vec4 *v);
void  Vec4_normalize(const Vec4 *src, Vec4 *dest);

void Vec4_lerp(const Vec4 *a, const Vec4 *b, float t, Vec4 *dest);

#endif
