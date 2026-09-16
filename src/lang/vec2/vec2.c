#include "lang/vec2/vec2.h"

#include <math.h>

#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec2 (lang/vec2/vec2.c — defined in lang/vec2/vec2.h)
 * LEVEL: L2 — Behavior (2D spatial vector)
 * ============================================================================
 * 2D spatial vector with horizontal and vertical components.
 * ============================================================================
 */

Vec2 *Vec2_0(void) {
    Vec2 *v = (Vec2*) Memory_alloc(ID_VEC2, VEC2_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = 0.0f;
    (*v).vertical = 0.0f;
    return v;
}

Vec2 *Vec2_2(float horizontal, float vertical) {
    Vec2 *v = (Vec2*) Memory_alloc(ID_VEC2, VEC2_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    return v;
}

void Vec2_free(Vec2 *v) {
    if (v) Memory_free(v);
}

float Vec2_getRight(const Vec2 *v) { return v ? (*v).horizontal : 0.0f; }
void  Vec2_setRight(Vec2 *v, float val) { if (v) (*v).horizontal = val; }
float Vec2_getLeft(const Vec2 *v)  { return v ? -(*v).horizontal : 0.0f; }
void  Vec2_setLeft(Vec2 *v, float val)  { if (v) (*v).horizontal = -val; }

float Vec2_getUp(const Vec2 *v)   { return v ? (*v).vertical : 0.0f; }
void  Vec2_setUp(Vec2 *v, float val)   { if (v) (*v).vertical = val; }
float Vec2_getDown(const Vec2 *v) { return v ? -(*v).vertical : 0.0f; }
void  Vec2_setDown(Vec2 *v, float val) { if (v) (*v).vertical = -val; }

float Vec2_getX(const Vec2 *v) { return v ? (*v).horizontal : 0.0f; }
void  Vec2_setX(Vec2 *v, float x) { if (v) (*v).horizontal = x; }
float Vec2_getY(const Vec2 *v) { return v ? (*v).vertical : 0.0f; }
void  Vec2_setY(Vec2 *v, float y) { if (v) (*v).vertical = y; }

float Vec2_getYInFrame(const Vec2 *v, CoordFrame2D frame) {
    if (!v) return 0.0f;
    return (frame == COORD_FRAME_2D_Y_DOWN) ? -(*v).vertical : (*v).vertical;
}

void Vec2_set(Vec2 *v, float horizontal, float vertical) {
    if (!v) return;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
}

void Vec2_copy(const Vec2 *src, Vec2 *dest) {
    if (!src || !dest) return;
    (*dest).horizontal = (*src).horizontal;
    (*dest).vertical = (*src).vertical;
}

void Vec2_add(const Vec2 *a, const Vec2 *b, Vec2 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal + (*b).horizontal;
    (*dest).vertical = (*a).vertical + (*b).vertical;
}

void Vec2_sub(const Vec2 *a, const Vec2 *b, Vec2 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal - (*b).horizontal;
    (*dest).vertical = (*a).vertical - (*b).vertical;
}

void Vec2_mul(const Vec2 *a, float scalar, Vec2 *dest) {
    if (!a || !dest) return;
    (*dest).horizontal = (*a).horizontal * scalar;
    (*dest).vertical = (*a).vertical * scalar;
}

void Vec2_div(const Vec2 *a, float scalar, Vec2 *dest) {
    if (!a || !dest || scalar == 0.0f) return;
    float inv = 1.0f / scalar;
    (*dest).horizontal = (*a).horizontal * inv;
    (*dest).vertical = (*a).vertical * inv;
}

float Vec2_dot(const Vec2 *a, const Vec2 *b) {
    if (!a || !b) return 0.0f;
    return (*a).horizontal * (*b).horizontal + (*a).vertical * (*b).vertical;
}

float Vec2_lengthSquared(const Vec2 *v) {
    if (!v) return 0.0f;
    return (*v).horizontal * (*v).horizontal + (*v).vertical * (*v).vertical;
}

float Vec2_length(const Vec2 *v) {
    return StrictMath_sqrt(Vec2_lengthSquared(v));
}

void Vec2_normalize(const Vec2 *src, Vec2 *dest) {
    if (!src || !dest) return;
    float lenSq = Vec2_lengthSquared(src);
    if (lenSq < 1e-12f) {
        (*dest).horizontal = 0.0f;
        (*dest).vertical = 0.0f;
        return;
    }
    float invLen = StrictMath_invSqrt(lenSq);
    Vec2_mul(src, invLen, dest);
}

void Vec2_perpendicular(const Vec2 *src, Vec2 *dest) {
    if (!src || !dest) return;
    float h = (*src).horizontal;
    float v = (*src).vertical;
    (*dest).horizontal = -v;
    (*dest).vertical = h;
}

float Vec2_distance(const Vec2 *a, const Vec2 *b) {
    if (!a || !b) return 0.0f;
    float dx = (*a).horizontal - (*b).horizontal;
    float dy = (*a).vertical - (*b).vertical;
    return StrictMath_sqrt(dx * dx + dy * dy);
}

float Vec2_angle(const Vec2 *a, const Vec2 *b) {
    if (!a || !b) return 0.0f;
    float d = Vec2_dot(a, b);
    float l = Vec2_length(a) * Vec2_length(b);
    if (l < 1e-12f) return 0.0f;
    return StrictMath_acos(StrictMath_clamp(d / l, -1.0f, 1.0f));
}

void Vec2_lerp(const Vec2 *a, const Vec2 *b, float t, Vec2 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = StrictMath_lerp((*a).horizontal, (*b).horizontal, t);
    (*dest).vertical = StrictMath_lerp((*a).vertical, (*b).vertical, t);
}
