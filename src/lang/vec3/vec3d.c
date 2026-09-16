#include "lang/vec3/vec3d.h"

#include <math.h>

#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec3d (lang/vec3/vec3d.c — defined in lang/vec3/vec3d.h)
 * LEVEL: L2 — Behavior (32-byte SIMD double-precision 3D vector)
 * ============================================================================
 * Ultra-precision 3D spatial vector for orbital and high-fidelity physics.
 * ============================================================================
 */

Vec3d *Vec3d_0(void) {
    Vec3d *v = (Vec3d*) Memory_alloc(ID_VEC3, VEC3D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = 0.0;
    (*v).vertical = 0.0;
    (*v).depth = 0.0;
    (*v).frame = (uint64_t) COORD_FRAME_DEFAULT;
    return v;
}

Vec3d *Vec3d_3(double horizontal, double vertical, double depth) {
    Vec3d *v = (Vec3d*) Memory_alloc(ID_VEC3, VEC3D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).frame = (uint64_t) COORD_FRAME_DEFAULT;
    return v;
}

Vec3d *Vec3d_4(double horizontal, double vertical, double depth, CoordFrame frame) {
    Vec3d *v = (Vec3d*) Memory_alloc(ID_VEC3, VEC3D_BYTES);
    if (!v) return nullptr;
    (*v).horizontal = horizontal;
    (*v).vertical = vertical;
    (*v).depth = depth;
    (*v).frame = CoordFrame_isValid(frame) ? (uint64_t) frame : (uint64_t) COORD_FRAME_DEFAULT;
    return v;
}

void Vec3d_free(Vec3d *v) {
    if (v) Memory_free(v);
}

double Vec3d_getRight(const Vec3d *v) { return v ? (*v).horizontal : 0.0; }
void   Vec3d_setRight(Vec3d *v, double val) { if (v) (*v).horizontal = val; }
double Vec3d_getLeft(const Vec3d *v) { return v ? -(*v).horizontal : 0.0; }
void   Vec3d_setLeft(Vec3d *v, double val) { if (v) (*v).horizontal = -val; }

double Vec3d_getUp(const Vec3d *v) { return v ? (*v).vertical : 0.0; }
void   Vec3d_setUp(Vec3d *v, double val) { if (v) (*v).vertical = val; }
double Vec3d_getDown(const Vec3d *v) { return v ? -(*v).vertical : 0.0; }
void   Vec3d_setDown(Vec3d *v, double val) { if (v) (*v).vertical = -val; }

double Vec3d_getFront(const Vec3d *v) { return v ? (*v).depth : 0.0; }
void   Vec3d_setFront(Vec3d *v, double val) { if (v) (*v).depth = val; }
double Vec3d_getBack(const Vec3d *v) { return v ? -(*v).depth : 0.0; }
void   Vec3d_setBack(Vec3d *v, double val) { if (v) (*v).depth = -val; }

double Vec3d_getX(const Vec3d *v) {
    if (!v) return 0.0;
    CoordFrame f = (CoordFrame) (*v).frame;
    return (double) CoordFrame_getAxisSign(f, 0) * (*v).data[CoordFrame_getAxisIndex(f, 0)];
}

double Vec3d_getY(const Vec3d *v) {
    if (!v) return 0.0;
    CoordFrame f = (CoordFrame) (*v).frame;
    return (double) CoordFrame_getAxisSign(f, 1) * (*v).data[CoordFrame_getAxisIndex(f, 1)];
}

double Vec3d_getZ(const Vec3d *v) {
    if (!v) return 0.0;
    CoordFrame f = (CoordFrame) (*v).frame;
    return (double) CoordFrame_getAxisSign(f, 2) * (*v).data[CoordFrame_getAxisIndex(f, 2)];
}

void Vec3d_add(const Vec3d *a, const Vec3d *b, Vec3d *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal + (*b).horizontal;
    (*dest).vertical = (*a).vertical + (*b).vertical;
    (*dest).depth = (*a).depth + (*b).depth;
    (*dest).frame = (*a).frame;
}

void Vec3d_sub(const Vec3d *a, const Vec3d *b, Vec3d *dest) {
    if (!a || !b || !dest) return;
    (*dest).horizontal = (*a).horizontal - (*b).horizontal;
    (*dest).vertical = (*a).vertical - (*b).vertical;
    (*dest).depth = (*a).depth - (*b).depth;
    (*dest).frame = (*a).frame;
}

void Vec3d_mul(const Vec3d *a, double scalar, Vec3d *dest) {
    if (!a || !dest) return;
    (*dest).horizontal = (*a).horizontal * scalar;
    (*dest).vertical = (*a).vertical * scalar;
    (*dest).depth = (*a).depth * scalar;
    (*dest).frame = (*a).frame;
}

double Vec3d_dot(const Vec3d *a, const Vec3d *b) {
    if (!a || !b) return 0.0;
    return (*a).horizontal * (*b).horizontal +
           (*a).vertical * (*b).vertical +
           (*a).depth * (*b).depth;
}

double Vec3d_length(const Vec3d *v) {
    return StrictMath_sqrtD(Vec3d_dot(v, v));
}

void Vec3d_normalize(const Vec3d *src, Vec3d *dest) {
    if (!src || !dest) return;
    double d = Vec3d_dot(src, src);
    if (d < 1e-20) {
        (*dest).horizontal = 0.0;
        (*dest).vertical = 0.0;
        (*dest).depth = 0.0;
        (*dest).frame = (*src).frame;
        return;
    }
    double inv = StrictMath_invSqrtD(d);
    Vec3d_mul(src, inv, dest);
}
