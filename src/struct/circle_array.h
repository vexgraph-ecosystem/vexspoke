#ifndef STRUCT_CIRCLE_ARRAY_H
#define STRUCT_CIRCLE_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "struct/collection.h"

// struct/circle_array.h — 2D Circular Matrix evaluated via the Pythagorean Theorem.
//
// Represents a 2D circle bounded by a square matrix of size (2*radius + 1)^2.
// Valid cells satisfy: (gridX - radius)^2 + (gridY - radius)^2 <= radius^2.
// Alternatively accessible via center-relative offsets (dx, dy) where dx^2 + dy^2 <= radius^2.

typedef struct CircleArray {
    Collection collection;   // Base collection header (typeId, activeCount, etc.)
    int32_t    radius;       // Radius in cells
    int32_t    diameter;     // 2 * radius + 1
} CircleArray;

// Allocation: creates a CircleArray with radius and element class.
CircleArray *CircleArray_create(int32_t radius, uint32_t elementClass);
CircleArray *CircleArray_createWithStride(int32_t radius, uint32_t elementClass, size_t stride);
void CircleArray_free(CircleArray *self);

int32_t CircleArray_radius(const CircleArray *self);
int32_t CircleArray_diameter(const CircleArray *self);
size_t  CircleArray_validCellCount(const CircleArray *self);

// Pythagorean containment tests
bool CircleArray_containsGrid(const CircleArray *self, int32_t gridX, int32_t gridY);
bool CircleArray_containsOffset(const CircleArray *self, int32_t dx, int32_t dy);
int64_t CircleArray_distanceSquaredGrid(const CircleArray *self, int32_t gridX, int32_t gridY);
int64_t CircleArray_distanceSquaredOffset(int32_t dx, int32_t dy);

// Cell access by grid coordinates [0, diameter - 1] (Dest-last order: get writes to dest, set reads from src)
bool CircleArray_getGrid(const CircleArray *self, int32_t gridX, int32_t gridY, void *dest);
bool CircleArray_setGrid(CircleArray *self, int32_t gridX, int32_t gridY, const void *src);

// Cell access by center-relative offsets [-radius, radius]
bool CircleArray_getOffset(const CircleArray *self, int32_t dx, int32_t dy, void *dest);
bool CircleArray_setOffset(CircleArray *self, int32_t dx, int32_t dy, const void *src);

// Raw pointer to cell slot inside circular matrix (nullptr if outside circle or bounds)
uint8_t *CircleArray_slotGrid(CircleArray *self, int32_t gridX, int32_t gridY);
uint8_t *CircleArray_slotOffset(CircleArray *self, int32_t dx, int32_t dy);

// Iteration callback across all valid circular cells
typedef void (*CircleArrayCellFn)(int32_t gridX, int32_t gridY, int32_t dx, int32_t dy, const void *element, void *userData);
void CircleArray_forEach(const CircleArray *self, CircleArrayCellFn callback, void *userData);

#endif
