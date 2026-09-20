#include "lang/vec4/vec4d.h"

#include <math.h>

#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec4d
 * ============================================================================
 * 32-byte SIMD 4D double-precision vector: horizontal/vertical/depth/w
 * components in an alignas(32) union with x/y/z/w, right/up/front aliases,
 * and a raw data[4] view. Directional accessors expose semantic axes whose
 * inverses are exact negations. All arithmetic is dest-last and null-guarded;
 * length uses strict double sqrt over the 4-component dot product. Arena-
 * allocated via Memory_alloc with the ID_VEC4 type tag.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec4d (lang/vec4/vec4d.c — defined in lang/vec4/vec4d.h)
 * LEVEL: L2 — Behavior (32-byte SIMD 4D double precision vector)
 * ============================================================================
 *
 * STRUCT FIELDS (Mirroring lang/vec4/vec4d.h):
 * ----------------------------------------------------------------------------
 *   Vec4d {
 *     alignas(32) union {
 *       struct { double horizontal; double vertical; double depth; double w; };
 *       struct { double x; double y; double z; double _w; };
 *       struct { double right; double up; double front; double _w2; };
 *       double data[4];
 *     };
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Vec4d_0(void)
 *   - Vec4d_4(horizontal, vertical, depth, w)
 *   - Vec4d_free(v)
 *
 * Public Setters: (.h)
 *   - Vec4d_setRight(v, val) / Vec4d_setLeft(v, val)
 *   - Vec4d_setUp(v, val) / Vec4d_setDown(v, val)
 *   - Vec4d_setFront(v, val) / Vec4d_setBack(v, val)
 *
 * Public Getters: (.h)
 *   - Vec4d_getRight(v) / Vec4d_getLeft(v)
 *   - Vec4d_getUp(v) / Vec4d_getDown(v)
 *   - Vec4d_getFront(v) / Vec4d_getBack(v)
 *
 * Public Core Functions: (.h)
 *   - Vec4d_add(a, b, dest)
 *   - Vec4d_sub(a, b, dest)
 *   - Vec4d_mul(a, scalar, dest)
 *   - Vec4d_dot(a, b)
 *   - Vec4d_length(v)
 * ============================================================================
 */

Vec4d *Vec4d_0(void) {
    Vec4d *v = (Vec4d*) Memory_alloc(ID_VEC4, VEC4D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = 0.0;
    (*v).vertical = 0.0;
    (*v).depth = 0.0;
    (*v).w = 0.0;
    return v;
}

Vec4d *Vec4d_4(double horizontal, double vertical, double depth, double w) {
    Vec4d *v = (Vec4d*) Memory_alloc(ID_VEC4, VEC4D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).w = w;
    return v;
}

void Vec4d_free(Vec4d *v) {
    if (v) Memory_free(v);
}

double Vec4d_getRight(const Vec4d *v) { return v ? (*v).horizontal : 0.0; }
void   Vec4d_setRight(Vec4d *v, double val) { if (v) (*v).horizontal = val; }
double Vec4d_getLeft(const Vec4d *v)  { return v ? -(*v).horizontal : 0.0; }
void   Vec4d_setLeft(Vec4d *v, double val)  { if (v) (*v).horizontal = -val; }

double Vec4d_getUp(const Vec4d *v)   { return v ? (*v).vertical : 0.0; }
void   Vec4d_setUp(Vec4d *v, double val)   { if (v) (*v).vertical = val; }
double Vec4d_getDown(const Vec4d *v) { return v ? -(*v).vertical : 0.0; }
void   Vec4d_setDown(Vec4d *v, double val) { if (v) (*v).vertical = -val; }

double Vec4d_getFront(const Vec4d *v) { return v ? (*v).depth : 0.0; }
void   Vec4d_setFront(Vec4d *v, double val) { if (v) (*v).depth = val; }
double Vec4d_getBack(const Vec4d *v)  { return v ? -(*v).depth : 0.0; }
void   Vec4d_setBack(Vec4d *v, double val)  { if (v) (*v).depth = -val; }

void Vec4d_add(const Vec4d *a, const Vec4d *b, Vec4d *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal + (*b).horizontal;
    (*dest).vertical = (*a).vertical + (*b).vertical;
    (*dest).depth = (*a).depth + (*b).depth;
    (*dest).w = (*a).w + (*b).w;
}

void Vec4d_sub(const Vec4d *a, const Vec4d *b, Vec4d *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal - (*b).horizontal;
    (*dest).vertical = (*a).vertical - (*b).vertical;
    (*dest).depth = (*a).depth - (*b).depth;
    (*dest).w = (*a).w - (*b).w;
}

void Vec4d_mul(const Vec4d *a, double scalar, Vec4d *dest) {
    if (!a || !dest) return;
    (*dest).horizontal = (*a).horizontal * scalar;
    (*dest).vertical = (*a).vertical * scalar;
    (*dest).depth = (*a).depth * scalar;
    (*dest).w = (*a).w * scalar;
}

double Vec4d_dot(const Vec4d *a, const Vec4d *b) {
    if (!a || !b) return 0.0;
    return (*a).horizontal * (*b).horizontal +
           (*a).vertical * (*b).vertical +
           (*a).depth * (*b).depth +
           (*a).w * (*b).w;
}

double Vec4d_length(const Vec4d *v) {
    return StrictMath_sqrtD(Vec4d_dot(v, v));
}
