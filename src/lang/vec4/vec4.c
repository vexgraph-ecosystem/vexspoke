#include "lang/vec4/vec4.h"

#include <math.h>

#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec4 (lang/vec4/vec4.c — defined in lang/vec4/vec4.h)
 * LEVEL: L2 — Behavior (16-byte SIMD 4D vector)
 * ============================================================================
 * 4D vector with horizontal, vertical, depth, and w components.
 * ============================================================================
 */

Vec4 *Vec4_0(void) {
    Vec4 *v = (Vec4*) Memory_alloc(ID_VEC4, VEC4_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = 0.0f;
    (*v).vertical = 0.0f;
    (*v).depth = 0.0f;
    (*v).w = 0.0f;
    return v;
}

Vec4 *Vec4_4(float horizontal, float vertical, float depth, float w) {
    Vec4 *v = (Vec4*) Memory_alloc(ID_VEC4, VEC4_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).w = w;
    return v;
}

void Vec4_free(Vec4 *v) {
    if (v) Memory_free(v);
}

float Vec4_getRight(const Vec4 *v) { return v ? (*v).horizontal : 0.0f; }
void  Vec4_setRight(Vec4 *v, float val) { if (v) (*v).horizontal = val; }
float Vec4_getLeft(const Vec4 *v)  { return v ? -(*v).horizontal : 0.0f; }
void  Vec4_setLeft(Vec4 *v, float val)  { if (v) (*v).horizontal = -val; }

float Vec4_getUp(const Vec4 *v)   { return v ? (*v).vertical : 0.0f; }
void  Vec4_setUp(Vec4 *v, float val)   { if (v) (*v).vertical = val; }
float Vec4_getDown(const Vec4 *v) { return v ? -(*v).vertical : 0.0f; }
void  Vec4_setDown(Vec4 *v, float val) { if (v) (*v).vertical = -val; }

float Vec4_getFront(const Vec4 *v) { return v ? (*v).depth : 0.0f; }
void  Vec4_setFront(Vec4 *v, float val) { if (v) (*v).depth = val; }
float Vec4_getBack(const Vec4 *v)  { return v ? -(*v).depth : 0.0f; }
void  Vec4_setBack(Vec4 *v, float val)  { if (v) (*v).depth = -val; }

float Vec4_getX(const Vec4 *v) { return v ? (*v).horizontal : 0.0f; }
void  Vec4_setX(Vec4 *v, float x) { if (v) (*v).horizontal = x; }
float Vec4_getY(const Vec4 *v) { return v ? (*v).vertical : 0.0f; }
void  Vec4_setY(Vec4 *v, float y) { if (v) (*v).vertical = y; }
float Vec4_getZ(const Vec4 *v) { return v ? (*v).depth : 0.0f; }
void  Vec4_setZ(Vec4 *v, float z) { if (v) (*v).depth = z; }
float Vec4_getW(const Vec4 *v) { return v ? (*v).w : 0.0f; }
void  Vec4_setW(Vec4 *v, float w) { if (v) (*v).w = w; }

float Vec4_getXInFrame(const Vec4 *v, CoordFrame frame) {
    if (!v) return 0.0f;
    return CoordFrame_getAxisSign(frame, 0) * (*v).data[CoordFrame_getAxisIndex(frame, 0)];
}

float Vec4_getYInFrame(const Vec4 *v, CoordFrame frame) {
    if (!v) return 0.0f;
    return CoordFrame_getAxisSign(frame, 1) * (*v).data[CoordFrame_getAxisIndex(frame, 1)];
}

float Vec4_getZInFrame(const Vec4 *v, CoordFrame frame) {
    if (!v) return 0.0f;
    return CoordFrame_getAxisSign(frame, 2) * (*v).data[CoordFrame_getAxisIndex(frame, 2)];
}

void Vec4_set(Vec4 *v, float horizontal, float vertical, float depth, float w) {
    if (!v) return;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).w = w;
}

void Vec4_copy(const Vec4 *src, Vec4 *dest) {
    if (!src || !dest) return;
    (*dest).horizontal = (*src).horizontal;
    (*dest).vertical = (*src).vertical;
    (*dest).depth = (*src).depth;
    (*dest).w = (*src).w;
}

void Vec4_add(const Vec4 *a, const Vec4 *b, Vec4 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal + (*b).horizontal;
    (*dest).vertical = (*a).vertical + (*b).vertical;
    (*dest).depth = (*a).depth + (*b).depth;
    (*dest).w = (*a).w + (*b).w;
}

void Vec4_sub(const Vec4 *a, const Vec4 *b, Vec4 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal - (*b).horizontal;
    (*dest).vertical = (*a).vertical - (*b).vertical;
    (*dest).depth = (*a).depth - (*b).depth;
    (*dest).w = (*a).w - (*b).w;
}

void Vec4_mul(const Vec4 *a, float scalar, Vec4 *dest) {
    if (!a || !dest) return;
    (*dest).horizontal = (*a).horizontal * scalar;
    (*dest).vertical = (*a).vertical * scalar;
    (*dest).depth = (*a).depth * scalar;
    (*dest).w = (*a).w * scalar;
}

void Vec4_div(const Vec4 *a, float scalar, Vec4 *dest) {
    if (!a || !dest || scalar == 0.0f) return;
    float inv = 1.0f / scalar;
    (*dest).horizontal = (*a).horizontal * inv;
    (*dest).vertical = (*a).vertical * inv;
    (*dest).depth = (*a).depth * inv;
    (*dest).w = (*a).w * inv;
}

float Vec4_dot(const Vec4 *a, const Vec4 *b) {
    if (!a || !b) return 0.0f;
    return (*a).horizontal * (*b).horizontal +
           (*a).vertical * (*b).vertical +
           (*a).depth * (*b).depth +
           (*a).w * (*b).w;
}

float Vec4_lengthSquared(const Vec4 *v) {
    if (!v) return 0.0f;
    return (*v).horizontal * (*v).horizontal +
           (*v).vertical * (*v).vertical +
           (*v).depth * (*v).depth +
           (*v).w * (*v).w;
}

float Vec4_length(const Vec4 *v) {
    return StrictMath_sqrt(Vec4_lengthSquared(v));
}

void Vec4_normalize(const Vec4 *src, Vec4 *dest) {
    if (!src || !dest) return;
    float lenSq = Vec4_lengthSquared(src);
    if (lenSq < 1e-12f) {
        Vec4_set(dest, 0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }
    float invLen = StrictMath_invSqrt(lenSq);
    Vec4_mul(src, invLen, dest);
}

void Vec4_lerp(const Vec4 *a, const Vec4 *b, float t, Vec4 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = StrictMath_lerp((*a).horizontal, (*b).horizontal, t);
    (*dest).vertical = StrictMath_lerp((*a).vertical, (*b).vertical, t);
    (*dest).depth = StrictMath_lerp((*a).depth, (*b).depth, t);
    (*dest).w = StrictMath_lerp((*a).w, (*b).w, t);
}
