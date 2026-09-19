#include "lang/rect/rectangle.h"

#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Rectangle (lang/rect/rectangle.c — defined in lang/rect/rectangle.h)
 * LEVEL: L2 — Behavior (pixel-space rectangle geometry)
 * ============================================================================
 * Axis-aligned rectangle in native pixel space (the Pixel Coordinate
 * Contract: the Graphics API, layout buffers and hit-testing all speak
 * native hardware pixels; -1..1 NDC exists only inside vertex shaders).
 *
 * STRUCT FIELDS (Mirroring lang/rect/rectangle.h):
 * ----------------------------------------------------------------------------
 *   float x;       // top-left x, native pixels
 *   float y;       // top-left y, native pixels (Y-down)
 *   float width;   // extent along +X, native pixels
 *   float height;  // extent along +Y, native pixels
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Rectangle()                : Rectangle_0()
 *   - Rectangle(x, y, w, h)      : Rectangle_4(x, y, w, h)
 *
 * Setters:
 *   - Rectangle_setX(r, x)
 *   - Rectangle_setY(r, y)
 *   - Rectangle_setWidth(r, width)
 *   - Rectangle_setHeight(r, height)
 *   - Rectangle_set(r, x, y, w, h)
 *
 * Getters:
 *   - Rectangle_getX(r)          : float
 *   - Rectangle_getY(r)          : float
 *   - Rectangle_getWidth(r)      : float
 *   - Rectangle_getHeight(r)     : float
 *   - Rectangle_getLeft(r)       : float (= x)
 *   - Rectangle_getTop(r)        : float (= y)
 *   - Rectangle_getRight(r)      : float (= x + width)
 *   - Rectangle_getBottom(r)     : float (= y + height)
 *
 * Core Functions:
 *   - Rectangle_copy(src, dest)
 *   - Rectangle_translate(src, dx, dy, dest)
 *   - Rectangle_inflate(src, dx, dy, dest)
 *   - Rectangle_inset(src, l, t, r, b, dest)
 *   - Rectangle_intersection(a, b, dest)   : disjoint -> empty (0,0,0,0)
 *   - Rectangle_merge(a, b, dest)
 *   - Rectangle_isEmpty(r)                 : bool (w <= 0 || h <= 0)
 *   - Rectangle_containsPoint(r, px, py)   : bool (touching edges count)
 *   - Rectangle_containsRect(outer, inner) : bool (touching edges count)
 *   - Rectangle_intersects(a, b)           : bool
 *   - Rectangle_equals(a, b)               : bool
 * ============================================================================
 */

// CONSTRUCTORS
Rectangle *Rectangle_0(void) {
    Rectangle *r = (Rectangle*) Memory_alloc(ID_RECT, RECTANGLE_BYTES);
    if (!r) return nullptr;
    (*r).x = 0.0f;
    (*r).y = 0.0f;
    (*r).width = 0.0f;
    (*r).height = 0.0f;
    return r;
}

Rectangle *Rectangle_4(float x, float y, float width, float height) {
    Rectangle *r = (Rectangle*) Memory_alloc(ID_RECT, RECTANGLE_BYTES);
    if (!r) return nullptr;
    (*r).x = x;
    (*r).y = y;
    (*r).width = width;
    (*r).height = height;
    return r;
}

void Rectangle_free(Rectangle *r) {
    if (r) Memory_free(r);
}

// SETTERS
void Rectangle_setX(Rectangle *r, float x) { if (r) (*r).x = x; }
void Rectangle_setY(Rectangle *r, float y) { if (r) (*r).y = y; }
void Rectangle_setWidth(Rectangle *r, float width) { if (r) (*r).width = width; }
void Rectangle_setHeight(Rectangle *r, float height) { if (r) (*r).height = height; }

void Rectangle_set(Rectangle *r, float x, float y, float width, float height) {
    if (!r) return;
    (*r).x = x;
    (*r).y = y;
    (*r).width = width;
    (*r).height = height;
}

// GETTERS
float Rectangle_getX(const Rectangle *r) { return r ? (*r).x : 0.0f; }
float Rectangle_getY(const Rectangle *r) { return r ? (*r).y : 0.0f; }
float Rectangle_getWidth(const Rectangle *r) { return r ? (*r).width : 0.0f; }
float Rectangle_getHeight(const Rectangle *r) { return r ? (*r).height : 0.0f; }
float Rectangle_getLeft(const Rectangle *r) { return r ? (*r).x : 0.0f; }
float Rectangle_getTop(const Rectangle *r) { return r ? (*r).y : 0.0f; }
float Rectangle_getRight(const Rectangle *r) { return r ? (*r).x + (*r).width : 0.0f; }
float Rectangle_getBottom(const Rectangle *r) { return r ? (*r).y + (*r).height : 0.0f; }

// CORE FUNCTIONS
void Rectangle_copy(const Rectangle *src, Rectangle *dest) {
    if (!src || !dest) return;
    (*dest).x = (*src).x;
    (*dest).y = (*src).y;
    (*dest).width = (*src).width;
    (*dest).height = (*src).height;
}

void Rectangle_translate(const Rectangle *src, float dx, float dy, Rectangle *dest) {
    if (!src || !dest) return;
    (*dest).x = (*src).x + dx;
    (*dest).y = (*src).y + dy;
    (*dest).width = (*src).width;
    (*dest).height = (*src).height;
}

void Rectangle_inflate(const Rectangle *src, float dx, float dy, Rectangle *dest) {
    if (!src || !dest) return;
    (*dest).x = (*src).x - dx;
    (*dest).y = (*src).y - dy;
    (*dest).width = (*src).width + dx + dx;
    (*dest).height = (*src).height + dy + dy;
}

void Rectangle_inset(const Rectangle *src, float l, float t, float r, float b, Rectangle *dest) {
    if (!src || !dest) return;
    (*dest).x = (*src).x + l;
    (*dest).y = (*src).y + t;
    (*dest).width = (*src).width - l - r;
    (*dest).height = (*src).height - t - b;
    if ((*dest).width < 0.0f) (*dest).width = 0.0f;
    if ((*dest).height < 0.0f) (*dest).height = 0.0f;
}

void Rectangle_intersection(const Rectangle *a, const Rectangle *b, Rectangle *dest) {
    if (!a || !b || !dest) return;
    float l = ((*a).x > (*b).x) ? (*a).x : (*b).x;
    float t = ((*a).y > (*b).y) ? (*a).y : (*b).y;
    float rA = (*a).x + (*a).width;
    float rB = (*b).x + (*b).width;
    float bA = (*a).y + (*a).height;
    float bB = (*b).y + (*b).height;
    float r = (rA < rB) ? rA : rB;
    float bot = (bA < bB) ? bA : bB;
    if (r <= l || bot <= t) {
        (*dest).x = 0.0f;
        (*dest).y = 0.0f;
        (*dest).width = 0.0f;
        (*dest).height = 0.0f;
        return;
    }
    (*dest).x = l;
    (*dest).y = t;
    (*dest).width = r - l;
    (*dest).height = bot - t;
}

void Rectangle_merge(const Rectangle *a, const Rectangle *b, Rectangle *dest) {
    if (!a || !b || !dest) return;
    float l = ((*a).x < (*b).x) ? (*a).x : (*b).x;
    float t = ((*a).y < (*b).y) ? (*a).y : (*b).y;
    float rA = (*a).x + (*a).width;
    float rB = (*b).x + (*b).width;
    float bA = (*a).y + (*a).height;
    float bB = (*b).y + (*b).height;
    float r = (rA > rB) ? rA : rB;
    float bot = (bA > bB) ? bA : bB;
    (*dest).x = l;
    (*dest).y = t;
    (*dest).width = r - l;
    (*dest).height = bot - t;
}

bool Rectangle_isEmpty(const Rectangle *r) {
    if (!r) return true;
    return (*r).width <= 0.0f || (*r).height <= 0.0f;
}

bool Rectangle_containsPoint(const Rectangle *r, float px, float py) {
    if (!r) return false;
    if (px < (*r).x || py < (*r).y) return false;
    if (px > (*r).x + (*r).width || py > (*r).y + (*r).height) return false;
    return true;
}

bool Rectangle_containsRect(const Rectangle *outer, const Rectangle *inner) {
    if (!outer || !inner) return false;
    if ((*inner).x < (*outer).x || (*inner).y < (*outer).y) return false;
    if ((*inner).x + (*inner).width > (*outer).x + (*outer).width) return false;
    if ((*inner).y + (*inner).height > (*outer).y + (*outer).height) return false;
    return true;
}

bool Rectangle_intersects(const Rectangle *a, const Rectangle *b) {
    if (!a || !b) return false;
    if ((*a).x + (*a).width <= (*b).x) return false;
    if ((*b).x + (*b).width <= (*a).x) return false;
    if ((*a).y + (*a).height <= (*b).y) return false;
    if ((*b).y + (*b).height <= (*a).y) return false;
    return true;
}

bool Rectangle_equals(const Rectangle *a, const Rectangle *b) {
    if (!a || !b) return false;
    return (*a).x == (*b).x && (*a).y == (*b).y &&
           (*a).width == (*b).width && (*a).height == (*b).height;
}