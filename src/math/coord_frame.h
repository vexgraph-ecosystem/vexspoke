#ifndef MATH_COORD_FRAME_H
#define MATH_COORD_FRAME_H

#include <stdbool.h>
#include <stdint.h>

// math/coord_frame.h — 12 Primary Spatial Coordinate Frames.
//
// Single Class Per File Law: CoordFrame.
//
// Defines the 12 spatial orientations based on primary Up axis (+/- Y, +/- Z, +/- X)
// and Handedness (Left-handed vs Right-handed).
//
// Default convention is COORD_FRAME_Y_UP_LEFT (Unity convention: +Y Up, +X Right, +Z Depth/Forward).
//
// Spatial semantic axes:
//   Axis 0 (Horizontal): Negative = Left,  Positive = Right
//   Axis 1 (Vertical):   Negative = Down,  Positive = Up
//   Axis 2 (Depth):      Negative = Back,  Positive = Front

typedef enum CoordFrame {
    COORD_FRAME_Y_UP_LEFT = 0,     // DEFAULT: +Y Up,   +X Right, +Z Front (Unity)
    COORD_FRAME_Y_UP_RIGHT,        // +Y Up,   +X Right, -Z Front (OpenGL / Vulkan eye space)
    COORD_FRAME_Z_UP_LEFT,         // +Z Up,   +Y Right, +X Front
    COORD_FRAME_Z_UP_RIGHT,        // +Z Up,   +X Right, +Y Front (Unreal / Blender)
    COORD_FRAME_X_UP_LEFT,         // +X Up,   +Z Right, +Y Front
    COORD_FRAME_X_UP_RIGHT,        // +X Up,   +Y Right, +Z Front
    COORD_FRAME_Y_DOWN_LEFT,       // -Y Up (Y Down), +X Right, +Z Front
    COORD_FRAME_Y_DOWN_RIGHT,      // -Y Up (Y Down), +X Right, -Z Front (Vulkan canvas/screen)
    COORD_FRAME_Z_DOWN_LEFT,       // -Z Up (Z Down), +Y Right, +X Front
    COORD_FRAME_Z_DOWN_RIGHT,      // -Z Up (Z Down), +X Right, +Y Front
    COORD_FRAME_X_DOWN_LEFT,       // -X Up (X Down), +Z Right, +Y Front
    COORD_FRAME_X_DOWN_RIGHT,      // -X Up (X Down), +Y Right, +Z Front
    COORD_FRAME_COUNT = 12
} CoordFrame;

#define COORD_FRAME_DEFAULT COORD_FRAME_Y_UP_LEFT

// Permutation & sign descriptor for branchless one-line indexing math:
// data[axis] * sign maps standard XYZ to the target spatial dimension.
typedef struct CoordBasis {
    int8_t axis[3]; // which spatial component (0=horizontal, 1=vertical, 2=depth)
    int8_t sign[3]; // +1 or -1 multiplier
} CoordBasis;

// Query the basis descriptor for a given coordinate frame.
CoordBasis CoordFrame_getBasis(CoordFrame frame);

// Retrieve component index and sign for X, Y, or Z in one call:
int8_t CoordFrame_getAxisIndex(CoordFrame frame, int xyzIndex);
float CoordFrame_getAxisSign(CoordFrame frame, int xyzIndex);

// Validate whether an integer represents a valid CoordFrame (0..11).
bool CoordFrame_isValid(uint32_t frame);

// Human-readable name for diagnostics.
const char *CoordFrame_name(CoordFrame frame);

#endif
