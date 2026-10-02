#ifndef LANG_POINT_POINT_H
#define LANG_POINT_POINT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "c23/constructor.h"

// lang/point/point.h — Integer Pixel Point.
//
// Single Class Per File Law: Point.
//
// Integer-precision 2D position in native pixel space (the Pixel Coordinate
// Contract: the Graphics API, layout buffers and hit-testing all speak
// native hardware pixels; -1..1 NDC exists only inside vertex shaders).
// int32_t members give element-wise accuracy for pixel snapping, software
// raster ops and integer geometry where floats buy nothing.
//
// Screen convention: +Y is DOWN in UI pixel space (the CoordFrame2D
// Y-down frame; Point does not carry a frame flag — pixel space is always
// top-left origin, matching CoreAnimation/Vulkan/Direct drawables).

typedef struct Point {
    int32_t x;  // horizontal pixel position (native pixels)
    int32_t y;  // vertical pixel position (native pixels, Y-down)
} Point;

#define POINT_BYTES 8u

// Constructors:
//   Point()        : Point_0()   — origin (0, 0)
//   Point(x, y)    : Point_2(x, y)
Point *Point_0(void);
Point *Point_2(int32_t x, int32_t y);
void Point_free(Point *p);

// Setters
void Point_setX(Point *p, int32_t x);
void Point_setY(Point *p, int32_t y);
void Point_set(Point *p, int32_t x, int32_t y);

// Getters (null-safe: 0 on null)
int32_t Point_getX(const Point *p);
int32_t Point_getY(const Point *p);

// Core functions (dest-last: (srcA, srcB, dest))
void Point_copy(const Point *src, Point *dest);
void Point_add(const Point *a, const Point *b, Point *dest);
void Point_sub(const Point *a, const Point *b, Point *dest);
void Point_scale(const Point *a, int32_t scalar, Point *dest);
void Point_negate(const Point *src, Point *dest);
void Point_min(const Point *a, const Point *b, Point *dest);
void Point_max(const Point *a, const Point *b, Point *dest);

// Metrics (null-safe: 0 / false on null)
int64_t Point_distanceSquared(const Point *a, const Point *b);
int64_t Point_manhattanDistance(const Point *a, const Point *b);
double Point_distance(const Point *a, const Point *b);
bool Point_equals(const Point *a, const Point *b);

#endif