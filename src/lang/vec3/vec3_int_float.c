#include "lang/vec3/vec3_int_float.h"

#include <math.h>

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec3IntFloat (lang/vec3/vec3_int_float.c — defined in lang/vec3/vec3_int_float.h)
 * LEVEL: L2 — Behavior (large-world sector + local float 3D vector)
 * ============================================================================
 * Represents coordinates across massive open worlds without precision loss.
 * ============================================================================
 */

Vec3IntFloat *Vec3IntFloat_0(void) {
    Vec3IntFloat *v = (Vec3IntFloat*) Memory_alloc(ID_VEC3, sizeof(Vec3IntFloat));
    if (!v) return nullptr;
    (*v).horizontal.sector = 0;
    (*v).horizontal.local = 0.0f;
    (*v).vertical.sector = 0;
    (*v).vertical.local = 0.0f;
    (*v).depth.sector = 0;
    (*v).depth.local = 0.0f;
    (*v).frame = (uint32_t) COORD_FRAME_DEFAULT;
    return v;
}

Vec3IntFloat *Vec3IntFloat_create(int32_t sH, float lH, int32_t sV, float lV, int32_t sD, float lD, CoordFrame frame) {
    Vec3IntFloat *v = (Vec3IntFloat*) Memory_alloc(ID_VEC3, sizeof(Vec3IntFloat));
    if (!v) return nullptr;
    (*v).horizontal.sector = sH;
    (*v).horizontal.local = lH;
    (*v).vertical.sector = sV;
    (*v).vertical.local = lV;
    (*v).depth.sector = sD;
    (*v).depth.local = lD;
    (*v).frame = CoordFrame_isValid(frame) ? (uint32_t) frame : (uint32_t) COORD_FRAME_DEFAULT;
    return v;
}

void Vec3IntFloat_free(Vec3IntFloat *v) {
    if (v) Memory_free(v);
}

static void rebalance_axis(CoordIntFloat *axis, float sectorSize) {
    if (sectorSize <= 0.0f) return;
    while ((*axis).local >= sectorSize) {
        (*axis).local -= sectorSize;
        (*axis).sector += 1;
    }
    while ((*axis).local < 0.0f) {
        (*axis).local += sectorSize;
        (*axis).sector -= 1;
    }
}

void Vec3IntFloat_rebalance(Vec3IntFloat *v, float sectorSize) {
    if (!v) return;
    rebalance_axis(&(*v).horizontal, sectorSize);
    rebalance_axis(&(*v).vertical, sectorSize);
    rebalance_axis(&(*v).depth, sectorSize);
}

float Vec3IntFloat_getRightLocal(const Vec3IntFloat *v) { return v ? (*v).horizontal.local : 0.0f; }
float Vec3IntFloat_getLeftLocal(const Vec3IntFloat *v)  { return v ? -(*v).horizontal.local : 0.0f; }
float Vec3IntFloat_getUpLocal(const Vec3IntFloat *v)    { return v ? (*v).vertical.local : 0.0f; }
float Vec3IntFloat_getDownLocal(const Vec3IntFloat *v)  { return v ? -(*v).vertical.local : 0.0f; }
float Vec3IntFloat_getFrontLocal(const Vec3IntFloat *v) { return v ? (*v).depth.local : 0.0f; }
float Vec3IntFloat_getBackLocal(const Vec3IntFloat *v)  { return v ? -(*v).depth.local : 0.0f; }
