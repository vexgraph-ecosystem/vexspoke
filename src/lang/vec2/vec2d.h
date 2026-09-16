#ifndef LANG_VEC2_VEC2D_H
#define LANG_VEC2_VEC2D_H

#include <stddef.h>
#include <stdint.h>

// lang/vec2/vec2d.h — 2D Double-Precision Vector.
//
// Single Class Per File Law: Vec2d.

typedef struct Vec2d {
    union {
        struct { double horizontal; double vertical; };
        struct { double x; double y; };
        struct { double right; double up; };
        double data[2];
    };
} Vec2d;

#define VEC2D_BYTES 16u

Vec2d *Vec2d_0(void);
Vec2d *Vec2d_2(double horizontal, double vertical);
void Vec2d_free(Vec2d *v);

double Vec2d_getRight(const Vec2d *v);
void   Vec2d_setRight(Vec2d *v, double val);
double Vec2d_getLeft(const Vec2d *v);
void   Vec2d_setLeft(Vec2d *v, double val);

double Vec2d_getUp(const Vec2d *v);
void   Vec2d_setUp(Vec2d *v, double val);
double Vec2d_getDown(const Vec2d *v);
void   Vec2d_setDown(Vec2d *v, double val);

void Vec2d_add(const Vec2d *a, const Vec2d *b, Vec2d *dest);
void Vec2d_sub(const Vec2d *a, const Vec2d *b, Vec2d *dest);
void Vec2d_mul(const Vec2d *a, double scalar, Vec2d *dest);
double Vec2d_dot(const Vec2d *a, const Vec2d *b);
double Vec2d_length(const Vec2d *v);

#endif
