#include "lang/vec3/vec3_int_float.h"

#include <math.h>

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Vec3IntFloat
 * ============================================================================
 * Large-world 3D vector that eliminates 32-bit floating-point jitter at
 * great distances by pairing an integer grid sector with a normalized local
 * float offset on each of the three axes (horizontal/vertical/depth). A
 * CoordFrame id records which spatial convention the components are stated
 * in. Vec3IntFloat_rebalance normalizes sector overflow: when |local| passes
 * sectorSize the sector bumps and the local offset wraps back into range, so
 * coordinates stay bounded and exact. Directional getters (right/left/up/
 * down/front/back) expose signed local components for input and camera math.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Vec3IntFloat (lang/vec3/vec3_int_float.c — defined in lang/vec3/vec3_int_float.h)
 * LEVEL: L2 — Behavior (large-world sector + local float 3D vector)
 * ============================================================================
 * Represents coordinates across massive open worlds without precision loss.
 *
 * STRUCT FIELDS (Mirroring lang/vec3/vec3_int_float.h):
 * ----------------------------------------------------------------------------
 *   CoordIntFloat {
 *     int32_t sector; // integer grid tile index
 *     float local;    // normalized offset within the sector
 *   }
 *   Vec3IntFloat {
 *     CoordIntFloat horizontal; // right/left axis (sector + local)
 *     CoordIntFloat vertical;   // up/down axis (sector + local)
 *     CoordIntFloat depth;      // front/back axis (sector + local)
 *     uint32_t frame;           // CoordFrame id (COORD_FRAME_DEFAULT when unset)
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Vec3IntFloat_0()
 *   - Vec3IntFloat_create(sH, lH, sV, lV, sD, lD, frame)
 *
 * Core Functions:
 *   - Vec3IntFloat_free(v)
 *   - Vec3IntFloat_rebalance(v, sectorSize)
 *
 * Private Core Functions: (.c static)
 *   - rebalance_axis(axis, sectorSize)
 *
 * Getters:
 *   - Vec3IntFloat_getRightLocal(v)
 *   - Vec3IntFloat_getLeftLocal(v)
 *   - Vec3IntFloat_getUpLocal(v)
 *   - Vec3IntFloat_getDownLocal(v)
 *   - Vec3IntFloat_getFrontLocal(v)
 *   - Vec3IntFloat_getBackLocal(v)
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
