#ifndef LANG_VEC4_VEC4D_H
#define LANG_VEC4_VEC4D_H

#include <stddef.h>
#include <stdint.h>

// lang/vec4/vec4d.h — 32-byte SIMD 4D Double-Precision Vector.
//
// Single Class Per File Law: Vec4d.

typedef struct Vec4d {
    alignas(32) union {
        struct { double horizontal; double vertical; double depth; double w; };
        struct { double x; double y; double z; double _w; };
        struct { double right; double up; double front; double _w2; };
        double data[4];
    };
} Vec4d;

#define VEC4D_BYTES 32u

Vec4d *Vec4d_0(void);
Vec4d *Vec4d_4(double horizontal, double vertical, double depth, double w);
void Vec4d_free(Vec4d *v);

double Vec4d_getRight(const Vec4d *v);
void   Vec4d_setRight(Vec4d *v, double val);
double Vec4d_getLeft(const Vec4d *v);
void   Vec4d_setLeft(Vec4d *v, double val);

double Vec4d_getUp(const Vec4d *v);
void   Vec4d_setUp(Vec4d *v, double val);
double Vec4d_getDown(const Vec4d *v);
void   Vec4d_setDown(Vec4d *v, double val);

double Vec4d_getFront(const Vec4d *v);
void   Vec4d_setFront(Vec4d *v, double val);
double Vec4d_getBack(const Vec4d *v);
void   Vec4d_setBack(Vec4d *v, double val);

void Vec4d_add(const Vec4d *a, const Vec4d *b, Vec4d *dest);
void Vec4d_sub(const Vec4d *a, const Vec4d *b, Vec4d *dest);
void Vec4d_mul(const Vec4d *a, double scalar, Vec4d *dest);
double Vec4d_dot(const Vec4d *a, const Vec4d *b);
double Vec4d_length(const Vec4d *v);

#endif
