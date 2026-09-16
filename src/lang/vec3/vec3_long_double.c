#include "lang/vec3/vec3_long_double.h"

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec3LongDouble (lang/vec3/vec3_long_double.c — defined in lang/vec3/vec3_long_double.h)
 * LEVEL: L2 — Behavior (cosmic scale int64 sector + double local 3D vector)
 * ============================================================================
 * Unbounded spatial coordinate representation for solar-system and galaxy scale.
 * ============================================================================
 */

Vec3LongDouble *Vec3LongDouble_0(void) {
    Vec3LongDouble *v = (Vec3LongDouble*) Memory_alloc(ID_VEC3, sizeof(Vec3LongDouble));
    if (!v) return nullptr;
    (*v).horizontal.sector = 0;
    (*v).horizontal.local = 0.0;
    (*v).vertical.sector = 0;
    (*v).vertical.local = 0.0;
    (*v).depth.sector = 0;
    (*v).depth.local = 0.0;
    (*v).frame = (uint64_t) COORD_FRAME_DEFAULT;
    return v;
}

Vec3LongDouble *Vec3LongDouble_create(int64_t sH, double lH, int64_t sV, double lV, int64_t sD, double lD, CoordFrame frame) {
    Vec3LongDouble *v = (Vec3LongDouble*) Memory_alloc(ID_VEC3, sizeof(Vec3LongDouble));
    if (!v) return nullptr;
    (*v).horizontal.sector = sH;
    (*v).horizontal.local = lH;
    (*v).vertical.sector = sV;
    (*v).vertical.local = lV;
    (*v).depth.sector = sD;
    (*v).depth.local = lD;
    (*v).frame = CoordFrame_isValid(frame) ? (uint64_t) frame : (uint64_t) COORD_FRAME_DEFAULT;
    return v;
}

void Vec3LongDouble_free(Vec3LongDouble *v) {
    if (v) Memory_free(v);
}

static void rebalance_axis_d(CoordLongDouble *axis, double sectorSize) {
    if (sectorSize <= 0.0) return;
    while ((*axis).local >= sectorSize) {
        (*axis).local -= sectorSize;
        (*axis).sector += 1;
    }
    while ((*axis).local < 0.0) {
        (*axis).local += sectorSize;
        (*axis).sector -= 1;
    }
}

void Vec3LongDouble_rebalance(Vec3LongDouble *v, double sectorSize) {
    if (!v) return;
    rebalance_axis_d(&(*v).horizontal, sectorSize);
    rebalance_axis_d(&(*v).vertical, sectorSize);
    rebalance_axis_d(&(*v).depth, sectorSize);
}

double Vec3LongDouble_getRightLocal(const Vec3LongDouble *v) { return v ? (*v).horizontal.local : 0.0; }
double Vec3LongDouble_getLeftLocal(const Vec3LongDouble *v)  { return v ? -(*v).horizontal.local : 0.0; }
double Vec3LongDouble_getUpLocal(const Vec3LongDouble *v)    { return v ? (*v).vertical.local : 0.0; }
double Vec3LongDouble_getDownLocal(const Vec3LongDouble *v)  { return v ? -(*v).vertical.local : 0.0; }
double Vec3LongDouble_getFrontLocal(const Vec3LongDouble *v) { return v ? (*v).depth.local : 0.0; }
double Vec3LongDouble_getBackLocal(const Vec3LongDouble *v)  { return v ? -(*v).depth.local : 0.0; }
