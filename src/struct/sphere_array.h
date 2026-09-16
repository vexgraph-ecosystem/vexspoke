#ifndef STRUCT_SPHERE_ARRAY_H
#define STRUCT_SPHERE_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "struct/collection.h"

// struct/sphere_array.h — 3D Spherical Voxel Matrix evaluated via 3D Pythagorean Theorem.
//
// Represents a 3D spherical volume bounded by a cubic matrix of size (2*radius + 1)^3.
// Valid voxels satisfy: (gx - r)^2 + (gy - r)^2 + (gz - r)^2 <= r^2.
// Alternatively accessible via center-relative offsets (dx, dy, dz) where dx^2 + dy^2 + dz^2 <= r^2.

typedef struct SphereArray {
    Collection collection;   // Base collection header (typeId, activeCount, etc.)
    int32_t    radius;       // Radius in voxels
    int32_t    diameter;     // 2 * radius + 1
} SphereArray;

// Allocation
SphereArray *SphereArray_create(int32_t radius, uint32_t elementClass);
SphereArray *SphereArray_createWithStride(int32_t radius, uint32_t elementClass, size_t stride);
void SphereArray_free(SphereArray *self);

int32_t SphereArray_radius(const SphereArray *self);
int32_t SphereArray_diameter(const SphereArray *self);
size_t  SphereArray_validVoxelCount(const SphereArray *self);

// 3D Pythagorean containment tests
bool SphereArray_containsGrid(const SphereArray *self, int32_t gx, int32_t gy, int32_t gz);
bool SphereArray_containsOffset(const SphereArray *self, int32_t dx, int32_t dy, int32_t dz);
int64_t SphereArray_distanceSquaredGrid(const SphereArray *self, int32_t gx, int32_t gy, int32_t gz);
int64_t SphereArray_distanceSquaredOffset(int32_t dx, int32_t dy, int32_t dz);

// Voxel access by grid coordinates [0, diameter - 1] (Dest-last order)
bool SphereArray_getGrid(const SphereArray *self, int32_t gx, int32_t gy, int32_t gz, void *dest);
bool SphereArray_setGrid(SphereArray *self, int32_t gx, int32_t gy, int32_t gz, const void *src);

// Voxel access by center-relative offsets [-radius, radius]
bool SphereArray_getOffset(const SphereArray *self, int32_t dx, int32_t dy, int32_t dz, void *dest);
bool SphereArray_setOffset(SphereArray *self, int32_t dx, int32_t dy, int32_t dz, const void *src);

uint8_t *SphereArray_slotGrid(SphereArray *self, int32_t gx, int32_t gy, int32_t gz);
uint8_t *SphereArray_slotOffset(SphereArray *self, int32_t dx, int32_t dy, int32_t dz);

// Iteration callback across all valid spherical voxels
typedef void (*SphereArrayVoxelFn)(int32_t gx, int32_t gy, int32_t gz, int32_t dx, int32_t dy, int32_t dz, const void *voxel, void *userData);
void SphereArray_forEach(const SphereArray *self, SphereArrayVoxelFn callback, void *userData);

#endif
