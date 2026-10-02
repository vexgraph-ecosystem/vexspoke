#ifndef LANG_RECT_RECTANGLE_H
#define LANG_RECT_RECTANGLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "c23/constructor.h"

// lang/rect/rectangle.h — Pixel-Space Rectangle.
//
// Single Class Per File Law: Rectangle.
//
// Axis-aligned rectangle in native pixel space (the Pixel Coordinate
// Contract: the Graphics API, layout buffers and hit-testing all speak
// native hardware pixels; -1..1 NDC exists only inside vertex shaders).
// Floats allow sub-pixel precision (crisp-edge snapping happens in the
// raster backends); integer positions use the Point class.
//
// Screen convention: +Y is DOWN (top-left origin, Y-down), matching
// CoreAnimation/Vulkan/Direct drawables. x/y are the TOP-LEFT corner.
// An empty rectangle is width <= 0 or height <= 0.

typedef struct Rectangle {
    float x;       // top-left x, native pixels
    float y;       // top-left y, native pixels (Y-down)
    float width;   // extent along +X, native pixels
    float height;  // extent along +Y, native pixels
} Rectangle;

#define RECTANGLE_BYTES 16u

// Constructors:
//   Rectangle()                 : Rectangle_0()             — empty at origin
//   Rectangle(x, y, w, h)       : Rectangle_4(x, y, w, h)
Rectangle *Rectangle_0(void);
Rectangle *Rectangle_4(float x, float y, float width, float height);
void Rectangle_free(Rectangle *r);

// Setters
void Rectangle_setX(Rectangle *r, float x);
void Rectangle_setY(Rectangle *r, float y);
void Rectangle_setWidth(Rectangle *r, float width);
void Rectangle_setHeight(Rectangle *r, float height);
void Rectangle_set(Rectangle *r, float x, float y, float width, float height);

// Getters (null-safe: 0 on null)
float Rectangle_getX(const Rectangle *r);
float Rectangle_getY(const Rectangle *r);
float Rectangle_getWidth(const Rectangle *r);
float Rectangle_getHeight(const Rectangle *r);
float Rectangle_getLeft(const Rectangle *r);
float Rectangle_getTop(const Rectangle *r);
float Rectangle_getRight(const Rectangle *r);
float Rectangle_getBottom(const Rectangle *r);

// Core functions (dest-last: (srcA, srcB, dest))
void Rectangle_copy(const Rectangle *src, Rectangle *dest);
void Rectangle_translate(const Rectangle *src, float dx, float dy, Rectangle *dest);
void Rectangle_inflate(const Rectangle *src, float dx, float dy, Rectangle *dest);
void Rectangle_inset(const Rectangle *src, float l, float t, float r, float b, Rectangle *dest);
void Rectangle_intersection(const Rectangle *a, const Rectangle *b, Rectangle *dest);
void Rectangle_merge(const Rectangle *a, const Rectangle *b, Rectangle *dest);

// Predicates (null-safe: false on null)
bool Rectangle_isEmpty(const Rectangle *r);
bool Rectangle_containsPoint(const Rectangle *r, float px, float py);
bool Rectangle_containsRect(const Rectangle *outer, const Rectangle *inner);
bool Rectangle_intersects(const Rectangle *a, const Rectangle *b);
bool Rectangle_equals(const Rectangle *a, const Rectangle *b);

#endif