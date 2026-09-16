#ifndef LANG_VEC2_VEC2_H
#define LANG_VEC2_VEC2_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "c23/constructor.h"

// lang/vec2/vec2.h — 2D Spatial Vector.
//
// Single Class Per File Law: Vec2.
//
// 2D spatial vector with horizontal and vertical dimensions.
//   - horizontal (x): Right (+), Left (-)
//   - vertical   (y): Up (+),    Down (-)
//
// Direct directional inverses:
//   getLeft() == -getRight()
//   getDown() == -getUp()

typedef enum CoordFrame2D {
    COORD_FRAME_2D_Y_UP = 0,   // Cartesian math (+Y Up, +X Right)
    COORD_FRAME_2D_Y_DOWN = 1  // UI / Canvas / Screen (+Y Down, +X Right)
} CoordFrame2D;

typedef struct Vec2 {
    union {
        struct { float horizontal; float vertical; };
        struct { float x; float y; };
        struct { float right; float up; };
        float data[2];
    };
} Vec2;

#define VEC2_BYTES 8u

Vec2 *Vec2_0(void);
Vec2 *Vec2_2(float horizontal, float vertical);
void Vec2_free(Vec2 *v);

// Directional Getters & Setters
float Vec2_getRight(const Vec2 *v);
void  Vec2_setRight(Vec2 *v, float val);
float Vec2_getLeft(const Vec2 *v);
void  Vec2_setLeft(Vec2 *v, float val);

float Vec2_getUp(const Vec2 *v);
void  Vec2_setUp(Vec2 *v, float val);
float Vec2_getDown(const Vec2 *v);
void  Vec2_setDown(Vec2 *v, float val);

float Vec2_getX(const Vec2 *v);
void  Vec2_setX(Vec2 *v, float x);
float Vec2_getY(const Vec2 *v);
void  Vec2_setY(Vec2 *v, float y);

float Vec2_getYInFrame(const Vec2 *v, CoordFrame2D frame);

void Vec2_set(Vec2 *v, float horizontal, float vertical);
void Vec2_copy(const Vec2 *src, Vec2 *dest);

void Vec2_add(const Vec2 *a, const Vec2 *b, Vec2 *dest);
void Vec2_sub(const Vec2 *a, const Vec2 *b, Vec2 *dest);
void Vec2_mul(const Vec2 *a, float scalar, Vec2 *dest);
void Vec2_div(const Vec2 *a, float scalar, Vec2 *dest);

float Vec2_dot(const Vec2 *a, const Vec2 *b);
float Vec2_lengthSquared(const Vec2 *v);
float Vec2_length(const Vec2 *v);
void  Vec2_normalize(const Vec2 *src, Vec2 *dest);
void  Vec2_perpendicular(const Vec2 *src, Vec2 *dest);
float Vec2_distance(const Vec2 *a, const Vec2 *b);
float Vec2_angle(const Vec2 *a, const Vec2 *b);
void  Vec2_lerp(const Vec2 *a, const Vec2 *b, float t, Vec2 *dest);

#endif
