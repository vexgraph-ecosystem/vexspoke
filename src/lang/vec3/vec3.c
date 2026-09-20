#include "lang/vec3/vec3.h"

#include <math.h>
#include <string.h>

#include "math/strict_math.h"
#include "math/fast_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec3
 * ============================================================================
 * Coordinate-agnostic 3D spatial vector: horizontal/vertical/depth components
 * with the owning CoordFrame stored as true embedded metadata in the 4th
 * float slot, preserving 16-byte SIMD alignment for NEON/SSE. Directional
 * accessors (right/left/up/down/front/back) expose semantic axes whose
 * inverses are exact negations, while frame-mapped X/Y/Z accessors translate
 * through CoordFrame axis index/sign tables. All arithmetic is dest-last and
 * null-guarded; normalize uses strict IEEE inv-sqrt while fastNormalize uses
 * the relaxed Quake approximation.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec3 (lang/vec3/vec3.c — defined in lang/vec3/vec3.h)
 * LEVEL: L2 — Behavior (16-byte SIMD spatial coordinate-agnostic 3D vector)
 * ============================================================================
 * Coordinate-agnostic 3D spatial vector carrying embedded CoordFrame metadata.
 * Components:
 *   - horizontal: Right (+) / Left (-)
 *   - vertical:   Up (+)    / Down (-)
 *   - depth:      Front (+) / Back (-)
 *
 * Direct directional inversion:
 *   getBack() == -getFront()
 *   getLeft() == -getRight()
 *   getDown() == -getUp()
 *
 * STRUCT FIELDS (Mirroring lang/vec3/vec3.h):
 * ----------------------------------------------------------------------------
 *   Vec3 {
 *     alignas(16) union {
 *       struct { float horizontal; float vertical; float depth; uint32_t frame; };
 *       struct { float x; float y; float z; uint32_t _frame; };
 *       struct { float right; float up; float front; uint32_t _f; };
 *       float data[4];
 *     };
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Vec3_0(void)
 *   - Vec3_3(horizontal, vertical, depth)
 *   - Vec3_4(horizontal, vertical, depth, frame)
 *   - Vec3_free(v)
 *
 * Public Setters: (.h)
 *   - Vec3_setRight(v, val) / Vec3_setLeft(v, val)
 *   - Vec3_setUp(v, val) / Vec3_setDown(v, val)
 *   - Vec3_setFront(v, val) / Vec3_setBack(v, val)
 *   - Vec3_setFrame(v, frame)
 *   - Vec3_setX(v, x) / Vec3_setY(v, y) / Vec3_setZ(v, z)
 *   - Vec3_set(v, horizontal, vertical, depth)
 *
 * Public Getters: (.h)
 *   - Vec3_getRight(v) / Vec3_getLeft(v)
 *   - Vec3_getUp(v) / Vec3_getDown(v)
 *   - Vec3_getFront(v) / Vec3_getBack(v)
 *   - Vec3_getFrame(v)
 *   - Vec3_getX(v) / Vec3_getY(v) / Vec3_getZ(v)
 *   - Vec3_getXInFrame(v, frame) / Vec3_getYInFrame(v, frame) / Vec3_getZInFrame(v, frame)
 *
 * Public Core Functions: (.h)
 *   - Vec3_copy(src, dest)
 *   - Vec3_toFrame(src, targetFrame, dest)
 *   - Vec3_add(a, b, dest) / Vec3_sub(a, b, dest)
 *   - Vec3_mul(a, scalar, dest) / Vec3_div(a, scalar, dest)
 *   - Vec3_dot(a, b)
 *   - Vec3_cross(a, b, dest)
 *   - Vec3_lengthSquared(v) / Vec3_length(v)
 *   - Vec3_normalize(src, dest) / Vec3_fastNormalize(src, dest)
 *   - Vec3_distance(a, b) / Vec3_angle(a, b)
 *   - Vec3_project(vector, onto, dest)
 *   - Vec3_reflect(incident, normal, dest)
 *   - Vec3_clamp(src, min_val, max_val, dest)
 *   - Vec3_abs(src, dest)
 *   - Vec3_lerp(a, b, t, dest)
 * ============================================================================
 */

Vec3 *Vec3_0(void) {
    Vec3 *v = (Vec3*) Memory_alloc(ID_VEC3, VEC3_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = 0.0f;
    (*v).vertical = 0.0f;
    (*v).depth = 0.0f;
    (*v).frame = (uint32_t) COORD_FRAME_DEFAULT;
    return v;
}

Vec3 *Vec3_3(float horizontal, float vertical, float depth) {
    Vec3 *v = (Vec3*) Memory_alloc(ID_VEC3, VEC3_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).frame = (uint32_t) COORD_FRAME_DEFAULT;
    return v;
}

Vec3 *Vec3_4(float horizontal, float vertical, float depth, CoordFrame frame) {
    Vec3 *v = (Vec3*) Memory_alloc(ID_VEC3, VEC3_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).frame = CoordFrame_isValid(frame) ? (uint32_t) frame : (uint32_t) COORD_FRAME_DEFAULT;
    return v;
}

void Vec3_free(Vec3 *v) {
    if (v) Memory_free(v);
}

float Vec3_getRight(const Vec3 *v) { return v ? (*v).horizontal : 0.0f; }
void  Vec3_setRight(Vec3 *v, float val) { if (v) (*v).horizontal = val; }
float Vec3_getLeft(const Vec3 *v) { return v ? -(*v).horizontal : 0.0f; }
void  Vec3_setLeft(Vec3 *v, float val) { if (v) (*v).horizontal = -val; }

float Vec3_getUp(const Vec3 *v) { return v ? (*v).vertical : 0.0f; }
void  Vec3_setUp(Vec3 *v, float val) { if (v) (*v).vertical = val; }
float Vec3_getDown(const Vec3 *v) { return v ? -(*v).vertical : 0.0f; }
void  Vec3_setDown(Vec3 *v, float val) { if (v) (*v).vertical = -val; }

float Vec3_getFront(const Vec3 *v) { return v ? (*v).depth : 0.0f; }
void  Vec3_setFront(Vec3 *v, float val) { if (v) (*v).depth = val; }
float Vec3_getBack(const Vec3 *v) { return v ? -(*v).depth : 0.0f; }
void  Vec3_setBack(Vec3 *v, float val) { if (v) (*v).depth = -val; }

CoordFrame Vec3_getFrame(const Vec3 *v) {
    return v ? (CoordFrame) (*v).frame : COORD_FRAME_DEFAULT;
}

void Vec3_setFrame(Vec3 *v, CoordFrame frame) {
    if (v && CoordFrame_isValid(frame)) {
        (*v).frame = (uint32_t) frame;
    }
}

float Vec3_getXInFrame(const Vec3 *v, CoordFrame frame) {
    if (!v) return 0.0f;
    return CoordFrame_getAxisSign(frame, 0) * (*v).data[CoordFrame_getAxisIndex(frame, 0)];
}

float Vec3_getYInFrame(const Vec3 *v, CoordFrame frame) {
    if (!v) return 0.0f;
    return CoordFrame_getAxisSign(frame, 1) * (*v).data[CoordFrame_getAxisIndex(frame, 1)];
}

float Vec3_getZInFrame(const Vec3 *v, CoordFrame frame) {
    if (!v) return 0.0f;
    return CoordFrame_getAxisSign(frame, 2) * (*v).data[CoordFrame_getAxisIndex(frame, 2)];
}

float Vec3_getX(const Vec3 *v) {
    if (!v) return 0.0f;
    return Vec3_getXInFrame(v, (CoordFrame) (*v).frame);
}

void Vec3_setX(Vec3 *v, float x) {
    if (!v) return;
    CoordFrame f = (CoordFrame) (*v).frame;
    int8_t idx = CoordFrame_getAxisIndex(f, 0);
    float sign = CoordFrame_getAxisSign(f, 0);
    (*v).data[idx] = x * sign;
}

float Vec3_getY(const Vec3 *v) {
    if (!v) return 0.0f;
    return Vec3_getYInFrame(v, (CoordFrame) (*v).frame);
}

void Vec3_setY(Vec3 *v, float y) {
    if (!v) return;
    CoordFrame f = (CoordFrame) (*v).frame;
    int8_t idx = CoordFrame_getAxisIndex(f, 1);
    float sign = CoordFrame_getAxisSign(f, 1);
    (*v).data[idx] = y * sign;
}

float Vec3_getZ(const Vec3 *v) {
    if (!v) return 0.0f;
    return Vec3_getZInFrame(v, (CoordFrame) (*v).frame);
}

void Vec3_setZ(Vec3 *v, float z) {
    if (!v) return;
    CoordFrame f = (CoordFrame) (*v).frame;
    int8_t idx = CoordFrame_getAxisIndex(f, 2);
    float sign = CoordFrame_getAxisSign(f, 2);
    (*v).data[idx] = z * sign;
}

void Vec3_set(Vec3 *v, float horizontal, float vertical, float depth) {
    if (!v) return;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
}

void Vec3_copy(const Vec3 *src, Vec3 *dest) {
    if (!src || !dest) return;
    (*dest).horizontal = (*src).horizontal;
    (*dest).vertical = (*src).vertical;
    (*dest).depth = (*src).depth;
    (*dest).frame = (*src).frame;
}

void Vec3_toFrame(const Vec3 *src, CoordFrame targetFrame, Vec3 *dest) {
    if (!src || !dest) return;
    if (!CoordFrame_isValid(targetFrame)) {
        targetFrame = COORD_FRAME_DEFAULT;
    }
    // Extract semantic directions from src
    float r = (*src).horizontal;
    float u = (*src).vertical;
    float f = (*src).depth;

    (*dest).horizontal = r;
    (*dest).vertical = u;
    (*dest).depth = f;
    (*dest).frame = (uint32_t) targetFrame;
}

void Vec3_add(const Vec3 *a, const Vec3 *b, Vec3 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal + (*b).horizontal;
    (*dest).vertical = (*a).vertical + (*b).vertical;
    (*dest).depth = (*a).depth + (*b).depth;
    (*dest).frame = (*a).frame;
}

void Vec3_sub(const Vec3 *a, const Vec3 *b, Vec3 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal - (*b).horizontal;
    (*dest).vertical = (*a).vertical - (*b).vertical;
    (*dest).depth = (*a).depth - (*b).depth;
    (*dest).frame = (*a).frame;
}

void Vec3_mul(const Vec3 *a, float scalar, Vec3 *dest) {
    if (!a || !dest) return;
    (*dest).horizontal = (*a).horizontal * scalar;
    (*dest).vertical = (*a).vertical * scalar;
    (*dest).depth = (*a).depth * scalar;
    (*dest).frame = (*a).frame;
}

void Vec3_div(const Vec3 *a, float scalar, Vec3 *dest) {
    if (!a || !dest || scalar == 0.0f) return;
    float inv = 1.0f / scalar;
    (*dest).horizontal = (*a).horizontal * inv;
    (*dest).vertical = (*a).vertical * inv;
    (*dest).depth = (*a).depth * inv;
    (*dest).frame = (*a).frame;
}

float Vec3_dot(const Vec3 *a, const Vec3 *b) {
    if (!a || !b) return 0.0f;
    return (*a).horizontal * (*b).horizontal +
           (*a).vertical * (*b).vertical +
           (*a).depth * (*b).depth;
}

void Vec3_cross(const Vec3 *a, const Vec3 *b, Vec3 *dest) {
    if (!a || !b || !dest) return;
    float x1 = (*a).horizontal, y1 = (*a).vertical, z1 = (*a).depth;
    float x2 = (*b).horizontal, y2 = (*b).vertical, z2 = (*b).depth;
    (*dest).horizontal = y1 * z2 - z1 * y2;
    (*dest).vertical = z1 * x2 - x1 * z2;
    (*dest).depth = x1 * y2 - y1 * x2;
    (*dest).frame = (*a).frame;
}

float Vec3_lengthSquared(const Vec3 *v) {
    if (!v) return 0.0f;
    return (*v).horizontal * (*v).horizontal +
           (*v).vertical * (*v).vertical +
           (*v).depth * (*v).depth;
}

float Vec3_length(const Vec3 *v) {
    return StrictMath_sqrt(Vec3_lengthSquared(v));
}

void Vec3_normalize(const Vec3 *src, Vec3 *dest) {
    if (!src || !dest) return;
    float lenSq = Vec3_lengthSquared(src);
    if (lenSq < 1e-12f) {
        (*dest).horizontal = 0.0f;
        (*dest).vertical = 0.0f;
        (*dest).depth = 0.0f;
        (*dest).frame = (*src).frame;
        return;
    }
    float invLen = StrictMath_invSqrt(lenSq);
    Vec3_mul(src, invLen, dest);
}

void Vec3_fastNormalize(const Vec3 *src, Vec3 *dest) {
    if (!src || !dest) return;
    float lenSq = Vec3_lengthSquared(src);
    if (lenSq < 1e-8f) {
        (*dest).horizontal = 0.0f;
        (*dest).vertical = 0.0f;
        (*dest).depth = 0.0f;
        (*dest).frame = (*src).frame;
        return;
    }
    float invLen = FastMath_invSqrt(lenSq);
    Vec3_mul(src, invLen, dest);
}

float Vec3_distance(const Vec3 *a, const Vec3 *b) {
    if (!a || !b) return 0.0f;
    float dx = (*a).horizontal - (*b).horizontal;
    float dy = (*a).vertical - (*b).vertical;
    float dz = (*a).depth - (*b).depth;
    return StrictMath_sqrt(dx * dx + dy * dy + dz * dz);
}

float Vec3_angle(const Vec3 *a, const Vec3 *b) {
    if (!a || !b) return 0.0f;
    float d = Vec3_dot(a, b);
    float l = Vec3_length(a) * Vec3_length(b);
    if (l < 1e-12f) return 0.0f;
    float cosVal = StrictMath_clamp(d / l, -1.0f, 1.0f);
    return StrictMath_acos(cosVal);
}

void Vec3_project(const Vec3 *vector, const Vec3 *onto, Vec3 *dest) {
    if (!vector || !onto || !dest) return;
    float ontoLenSq = Vec3_lengthSquared(onto);
    if (ontoLenSq < 1e-12f) {
        Vec3_set(dest, 0.0f, 0.0f, 0.0f);
        return;
    }
    float scale = Vec3_dot(vector, onto) / ontoLenSq;
    Vec3_mul(onto, scale, dest);
}

void Vec3_reflect(const Vec3 *incident, const Vec3 *normal, Vec3 *dest) {
    if (!incident || !normal || !dest) return;
    float factor = 2.0f * Vec3_dot(incident, normal);
    (*dest).horizontal = (*incident).horizontal - factor * (*normal).horizontal;
    (*dest).vertical = (*incident).vertical - factor * (*normal).vertical;
    (*dest).depth = (*incident).depth - factor * (*normal).depth;
    (*dest).frame = (*incident).frame;
}

void Vec3_clamp(const Vec3 *src, float min_val, float max_val, Vec3 *dest) {
    if (!src || !dest) return;
    (*dest).horizontal = StrictMath_clamp((*src).horizontal, min_val, max_val);
    (*dest).vertical = StrictMath_clamp((*src).vertical, min_val, max_val);
    (*dest).depth = StrictMath_clamp((*src).depth, min_val, max_val);
    (*dest).frame = (*src).frame;
}

void Vec3_abs(const Vec3 *src, Vec3 *dest) {
    if (!src || !dest) return;
    (*dest).horizontal = StrictMath_abs((*src).horizontal);
    (*dest).vertical = StrictMath_abs((*src).vertical);
    (*dest).depth = StrictMath_abs((*src).depth);
    (*dest).frame = (*src).frame;
}

void Vec3_lerp(const Vec3 *a, const Vec3 *b, float t, Vec3 *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = StrictMath_lerp((*a).horizontal, (*b).horizontal, t);
    (*dest).vertical = StrictMath_lerp((*a).vertical, (*b).vertical, t);
    (*dest).depth = StrictMath_lerp((*a).depth, (*b).depth, t);
    (*dest).frame = (*a).frame;
}
