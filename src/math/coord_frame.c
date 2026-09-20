#include "math/coord_frame.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: CoordFrame
 * ============================================================================
 * Defines and resolves the 12 canonical 3D coordinate frames formed by the
 * primary Up axis (+/- Y, +/- Z, +/- X) and handedness (Left vs Right). Each
 * frame resolves to a CoordBasis — an axis permutation plus per-component
 * sign — that maps canonical spatial axes (0 horizontal, 1 vertical, 2 depth)
 * to engine XYZ with branchless one-line indexing math. The basis and name
 * tables are static const data, so the class is procedural and allocation-
 * free; out-of-range frames fail closed to the default Y_UP_LEFT basis or
 * "UNKNOWN". Default frame is COORD_FRAME_Y_UP_LEFT (Unity convention).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: CoordFrame (math/coord_frame.c — defined in math/coord_frame.h)
 * LEVEL: L2 — Behavior (12 spatial coordinate frames & basis resolution)
 * ============================================================================
 * Defines and resolves the 12 canonical 3D coordinate frames based on Up axis
 * (+/- Y, +/- Z, +/- X) and Handedness (Left vs Right).
 *
 * Canonical spatial axes in memory:
 *   Axis 0 (Horizontal): Negative = Left,  Positive = Right
 *   Axis 1 (Vertical):   Negative = Down,  Positive = Up
 *   Axis 2 (Depth):      Negative = Back,  Positive = Front
 *
 * Default frame is COORD_FRAME_Y_UP_LEFT (Unity convention).
 *
 * STRUCT FIELDS (Mirroring math/coord_frame.h):
 * ----------------------------------------------------------------------------
 *   CoordBasis {
 *     int8_t axis[3]; // spatial component per XYZ (0=horizontal, 1=vertical, 2=depth)
 *     int8_t sign[3]; // +1 or -1 multiplier per component
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - CoordFrame_getBasis(frame)
 *   - CoordFrame_getAxisIndex(frame, xyzIndex)
 *   - CoordFrame_getAxisSign(frame, xyzIndex)
 *   - CoordFrame_isValid(frame)
 *   - CoordFrame_name(frame)
 * ============================================================================
 */

static const CoordBasis s_bases[COORD_FRAME_COUNT] = {
    // 0: COORD_FRAME_Y_UP_LEFT (Unity: +Y Up, +X Right, +Z Front)
    { .axis = {0, 1, 2}, .sign = { 1,  1,  1} },

    // 1: COORD_FRAME_Y_UP_RIGHT (OpenGL/Vulkan eye space: +Y Up, +X Right, -Z Front)
    { .axis = {0, 1, 2}, .sign = { 1,  1, -1} },

    // 2: COORD_FRAME_Z_UP_LEFT (+Z Up, +Y Right, +X Front)
    { .axis = {2, 0, 1}, .sign = { 1,  1,  1} },

    // 3: COORD_FRAME_Z_UP_RIGHT (Unreal / Blender: +Z Up, +X Right, +Y Front)
    { .axis = {0, 2, 1}, .sign = { 1,  1,  1} },

    // 4: COORD_FRAME_X_UP_LEFT (+X Up, +Z Right, +Y Front)
    { .axis = {1, 2, 0}, .sign = { 1,  1,  1} },

    // 5: COORD_FRAME_X_UP_RIGHT (+X Up, +Y Right, +Z Front)
    { .axis = {1, 0, 2}, .sign = { 1,  1,  1} },

    // 6: COORD_FRAME_Y_DOWN_LEFT (-Y Up / Y Down, +X Right, +Z Front)
    { .axis = {0, 1, 2}, .sign = { 1, -1,  1} },

    // 7: COORD_FRAME_Y_DOWN_RIGHT (Vulkan canvas/screen: -Y Up / Y Down, +X Right, -Z Front)
    { .axis = {0, 1, 2}, .sign = { 1, -1, -1} },

    // 8: COORD_FRAME_Z_DOWN_LEFT (-Z Up / Z Down, +Y Right, +X Front)
    { .axis = {2, 0, 1}, .sign = { 1,  1, -1} },

    // 9: COORD_FRAME_Z_DOWN_RIGHT (-Z Up / Z Down, +X Right, +Y Front)
    { .axis = {0, 2, 1}, .sign = { 1,  1, -1} },

    // 10: COORD_FRAME_X_DOWN_LEFT (-X Up / X Down, +Z Right, +Y Front)
    { .axis = {1, 2, 0}, .sign = {-1,  1,  1} },

    // 11: COORD_FRAME_X_DOWN_RIGHT (-X Up / X Down, +Y Right, +Z Front)
    { .axis = {1, 0, 2}, .sign = {-1,  1,  1} }
};

static const char *s_names[COORD_FRAME_COUNT] = {
    "Y_UP_LEFT",
    "Y_UP_RIGHT",
    "Z_UP_LEFT",
    "Z_UP_RIGHT",
    "X_UP_LEFT",
    "X_UP_RIGHT",
    "Y_DOWN_LEFT",
    "Y_DOWN_RIGHT",
    "Z_DOWN_LEFT",
    "Z_DOWN_RIGHT",
    "X_DOWN_LEFT",
    "X_DOWN_RIGHT"
};

CoordBasis CoordFrame_getBasis(CoordFrame frame) {
    if ((uint32_t) frame >= COORD_FRAME_COUNT) {
        return s_bases[COORD_FRAME_DEFAULT];
    }
    return s_bases[frame];
}

int8_t CoordFrame_getAxisIndex(CoordFrame frame, int xyzIndex) {
    if ((uint32_t) frame >= COORD_FRAME_COUNT || (uint32_t) xyzIndex >= 3) {
        return 0;
    }
    return s_bases[frame].axis[xyzIndex];
}

float CoordFrame_getAxisSign(CoordFrame frame, int xyzIndex) {
    if ((uint32_t) frame >= COORD_FRAME_COUNT || (uint32_t) xyzIndex >= 3) {
        return 1.0f;
    }
    return (float) s_bases[frame].sign[xyzIndex];
}

bool CoordFrame_isValid(uint32_t frame) {
    return frame < COORD_FRAME_COUNT;
}

const char *CoordFrame_name(CoordFrame frame) {
    if ((uint32_t) frame >= COORD_FRAME_COUNT) {
        return "UNKNOWN";
    }
    return s_names[frame];
}
