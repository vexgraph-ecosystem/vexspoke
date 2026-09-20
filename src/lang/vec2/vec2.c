#include "lang/vec2/vec2.h"

#include <math.h>

#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec2
 * ============================================================================
 * 2D spatial vector with horizontal and vertical components, arena-allocated
 * at VEC2_BYTES (8 bytes) with zero steady-state allocation. Provides
 * directional getters/setters (right/left/up/down/x/y), frame-aware Y access,
 * and dest-last arithmetic (add/sub/mul/div, normalize, perpendicular,
 * distance, angle, lerp) per the Dest-Last Law. The union layout aliases
 * horizontal/vertical, x/y, and right/up so all naming conventions read the
 * same two floats.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec2 (lang/vec2/vec2.c — defined in lang/vec2/vec2.h)
 * LEVEL: L2 — Behavior (2D spatial vector)
 * ============================================================================
 * 2D spatial vector with horizontal and vertical components.
 *
 * STRUCT FIELDS (Mirroring lang/vec2/vec2.h):
 * ----------------------------------------------------------------------------
 *   Vec2 {
 *     union {
 *       struct { float horizontal; float vertical; };
 *       struct { float x; float y; };
 *       struct { float right; float up; };
 *       float data[2];
 *     };
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Vec2()                    : Vec2_0()
 *   - Vec2(horizontal, vertical): Vec2_2(horizontal, vertical)
 *
 * Public Core Functions: (.h)
 *   - Vec2_free(v)
 *   - Vec2_set(v, horizontal, vertical)
 *   - Vec2_copy(src, dest)
 *   - Vec2_add(a, b, dest)
 *   - Vec2_sub(a, b, dest)
 *   - Vec2_mul(a, scalar, dest)
 *   - Vec2_div(a, scalar, dest)
 *   - Vec2_dot(a, b)
 *   - Vec2_lengthSquared(v)
 *   - Vec2_length(v)
 *   - Vec2_normalize(src, dest)
 *   - Vec2_perpendicular(src, dest)
 *   - Vec2_distance(a, b)
 *   - Vec2_angle(a, b)
 *   - Vec2_lerp(a, b, t, dest)
 *
 * Public Setters: (.h)
 *   - Vec2_setRight(v, val) / Vec2_setLeft(v, val)
 *   - Vec2_setUp(v, val) / Vec2_setDown(v, val)
 *   - Vec2_setX(v, x) / Vec2_setY(v, y)
 *
 * Public Getters: (.h)
 *   - Vec2_getRight(v) / Vec2_getLeft(v)
 *   - Vec2_getUp(v) / Vec2_getDown(v)
 *   - Vec2_getX(v) / Vec2_getY(v)
 *   - Vec2_getYInFrame(v, frame)
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
