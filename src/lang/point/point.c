#include "lang/point/point.h"

#include "annotation/overview.h"
#include "math/strict_math.h"
#include "nio/mem.h"
#include "oop/type.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Point (lang/point/point.c — defined in lang/point/point.h)
 * LEVEL: L2 — Behavior (integer pixel geometry)
 * ============================================================================
 * Integer-precision 2D position in native pixel space (the Pixel Coordinate
 * Contract: the Graphics API, layout buffers and hit-testing all speak
 * native hardware pixels; -1..1 NDC exists only inside vertex shaders).
 *
 * STRUCT FIELDS (Mirroring lang/point/point.h):
 * ----------------------------------------------------------------------------
 *   int32_t x;       // horizontal pixel position (native pixels)
 *   int32_t y;       // vertical pixel position (native pixels, Y-down)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Point()          : Point_0()
 *   - Point(x, y)      : Point_2(x, y)
 *
 * Setters:
 *   - Point_setX(p, x)
 *   - Point_setY(p, y)
 *   - Point_set(p, x, y)
 *
 * Getters:
 *   - Point_getX(p)                    : int32_t
 *   - Point_getY(p)                    : int32_t
 *
 * Core Functions:
 *   - Point_copy(src, dest)
 *   - Point_add(a, b, dest)
 *   - Point_sub(a, b, dest)
 *   - Point_scale(a, scalar, dest)
 *   - Point_negate(src, dest)
 *   - Point_min(a, b, dest)
 *   - Point_max(a, b, dest)
 *   - Point_distanceSquared(a, b)      : int64_t
 *   - Point_manhattanDistance(a, b)    : int64_t
 *   - Point_distance(a, b)             : double
 *   - Point_equals(a, b)               : bool
 * ============================================================================
 */

// CONSTRUCTORS
Point *Point_0(void) {
    Point *p = (Point*) Memory_alloc(ID_POINT, POINT_BYTES);
    if (!p) return nullptr;
    (*p).x = 0;
    (*p).y = 0;
    return p;
}

Point *Point_2(int32_t x, int32_t y) {
    Point *p = (Point*) Memory_alloc(ID_POINT, POINT_BYTES);
    if (!p) return nullptr;
    (*p).x = x;
    (*p).y = y;
    return p;
}

void Point_free(Point *p) {
    if (p) Memory_free(p);
}

// SETTERS
void Point_setX(Point *p, int32_t x) { if (p) (*p).x = x; }
void Point_setY(Point *p, int32_t y) { if (p) (*p).y = y; }
void Point_set(Point *p, int32_t x, int32_t y) {
    if (!p) return;
    (*p).x = x;
    (*p).y = y;
}

// GETTERS
int32_t Point_getX(const Point *p) { return p ? (*p).x : 0; }
int32_t Point_getY(const Point *p) { return p ? (*p).y : 0; }

// CORE FUNCTIONS
void Point_copy(const Point *src, Point *dest) {
    if (!src || !dest) return;
    (*dest).x = (*src).x;
    (*dest).y = (*src).y;
}

void Point_add(const Point *a, const Point *b, Point *dest) {
    if (!a || !b || !dest) return;
    (*dest).x = (*a).x + (*b).x;
    (*dest).y = (*a).y + (*b).y;
}

void Point_sub(const Point *a, const Point *b, Point *dest) {
    if (!a || !b || !dest) return;
    (*dest).x = (*a).x - (*b).x;
    (*dest).y = (*a).y - (*b).y;
}

void Point_scale(const Point *a, int32_t scalar, Point *dest) {
    if (!a || !dest) return;
    (*dest).x = (*a).x * scalar;
    (*dest).y = (*a).y * scalar;
}

void Point_negate(const Point *src, Point *dest) {
    if (!src || !dest) return;
    (*dest).x = -(*src).x;
    (*dest).y = -(*src).y;
}

void Point_min(const Point *a, const Point *b, Point *dest) {
    if (!a || !b || !dest) return;
    (*dest).x = ((*a).x < (*b).x) ? (*a).x : (*b).x;
    (*dest).y = ((*a).y < (*b).y) ? (*a).y : (*b).y;
}

void Point_max(const Point *a, const Point *b, Point *dest) {
    if (!a || !b || !dest) return;
    (*dest).x = ((*a).x > (*b).x) ? (*a).x : (*b).x;
    (*dest).y = ((*a).y > (*b).y) ? (*a).y : (*b).y;
}

int64_t Point_distanceSquared(const Point *a, const Point *b) {
    if (!a || !b) return 0;
    int64_t dx = (int64_t) (*a).x - (int64_t) (*b).x;
    int64_t dy = (int64_t) (*a).y - (int64_t) (*b).y;
    return dx * dx + dy * dy;
}

int64_t Point_manhattanDistance(const Point *a, const Point *b) {
    if (!a || !b) return 0;
    int64_t dx = (int64_t) (*a).x - (int64_t) (*b).x;
    int64_t dy = (int64_t) (*a).y - (int64_t) (*b).y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy;
}

double Point_distance(const Point *a, const Point *b) {
    int64_t sq = Point_distanceSquared(a, b);
    if (sq == 0) return 0.0;
    return StrictMath_sqrtD((double) sq);
}

bool Point_equals(const Point *a, const Point *b) {
    if (!a || !b) return false;
    return (*a).x == (*b).x && (*a).y == (*b).y;
}