#include "lang/vec2/vec2d.h"

#include <math.h>

#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec2d (lang/vec2/vec2d.c — defined in lang/vec2/vec2d.h)
 * LEVEL: L2 — Behavior (2D double precision spatial vector)
 * ============================================================================
 */

Vec2d *Vec2d_0(void) {
    Vec2d *v = (Vec2d*) Memory_alloc(ID_VEC2, VEC2D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = 0.0;
    (*v).vertical = 0.0;
    return v;
}

Vec2d *Vec2d_2(double horizontal, double vertical) {
    Vec2d *v = (Vec2d*) Memory_alloc(ID_VEC2, VEC2D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    return v;
}

void Vec2d_free(Vec2d *v) {
    if (v) Memory_free(v);
}

double Vec2d_getRight(const Vec2d *v) { return v ? (*v).horizontal : 0.0; }
void   Vec2d_setRight(Vec2d *v, double val) { if (v) (*v).horizontal = val; }
double Vec2d_getLeft(const Vec2d *v)  { return v ? -(*v).horizontal : 0.0; }
void   Vec2d_setLeft(Vec2d *v, double val)  { if (v) (*v).horizontal = -val; }

double Vec2d_getUp(const Vec2d *v)   { return v ? (*v).vertical : 0.0; }
void   Vec2d_setUp(Vec2d *v, double val)   { if (v) (*v).vertical = val; }
double Vec2d_getDown(const Vec2d *v) { return v ? -(*v).vertical : 0.0; }
void   Vec2d_setDown(Vec2d *v, double val) { if (v) (*v).vertical = -val; }

void Vec2d_add(const Vec2d *a, const Vec2d *b, Vec2d *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal + (*b).horizontal;
    (*dest).vertical = (*a).vertical + (*b).vertical;
}

void Vec2d_sub(const Vec2d *a, const Vec2d *b, Vec2d *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal - (*b).horizontal;
    (*dest).vertical = (*a).vertical - (*b).vertical;
}

void Vec2d_mul(const Vec2d *a, double scalar, Vec2d *dest) {
    if (!a || !dest) return;
    (*dest).horizontal = (*a).horizontal * scalar;
    (*dest).vertical = (*a).vertical * scalar;
}

double Vec2d_dot(const Vec2d *a, const Vec2d *b) {
    if (!a || !b) return 0.0;
    return (*a).horizontal * (*b).horizontal + (*a).vertical * (*b).vertical;
}

double Vec2d_length(const Vec2d *v) {
    if (!v) return 0.0;
    return StrictMath_sqrtD(Vec2d_dot(v, v));
}
