#include "struct/sphere_array.h"

#include <string.h>
#include "nio/mem.h"
#include "oop/stride.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: SphereArray
 * ============================================================================
 * 3D spherical voxel matrix evaluated via the 3D Pythagorean theorem: a cubic
 * bounding volume of (2*radius + 1)^3 voxels where only cells whose Euclidean
 * distance from center satisfies dx^2 + dy^2 + dz^2 <= radius^2 are valid.
 * A Collection header plus a flat data buffer hold the voxels; the active
 * count is precomputed at construction so iteration never re-tests the whole
 * cube. Grid accessors address voxels by [0, diameter) coordinates and offset
 * accessors by center-relative [-radius, radius] deltas; both bounds-check
 * before touching memory. forEach walks only the valid spherical shell.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: SphereArray (struct/sphere_array.c)
 * LEVEL: L2 — Off-Heap Spatial Container
 * ============================================================================
 * 3D Spherical Voxel Matrix evaluated via the 3D Pythagorean Theorem.
 *
 * Encapsulates a cubic bounding voxel volume of size (2*radius + 1)^3 where
 * only voxels whose Euclidean distance from center satisfies:
 * dx^2 + dy^2 + dz^2 <= radius^2
 * are considered valid and accessible.
 *
 * STRUCT FIELDS (Mirroring struct/sphere_array.h):
 * ----------------------------------------------------------------------------
 *   SphereArray {
 *     Collection collection; // base collection header (typeId, activeCount, stride, data)
 *     int32_t radius;        // radius in voxels
 *     int32_t diameter;      // 2 * radius + 1
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - SphereArray_create(radius, elementClass)
 *   - SphereArray_createWithStride(radius, elementClass, stride)
 *
 * Core Functions:
 *   - SphereArray_free(self)
 *   - SphereArray_containsGrid(self, gx, gy, gz)
 *   - SphereArray_containsOffset(self, dx, dy, dz)
 *   - SphereArray_distanceSquaredGrid(self, gx, gy, gz)
 *   - SphereArray_distanceSquaredOffset(dx, dy, dz)
 *   - SphereArray_slotGrid(self, gx, gy, gz)
 *   - SphereArray_slotOffset(self, dx, dy, dz)
 *   - SphereArray_getGrid(self, gx, gy, gz, dest)
 *   - SphereArray_setGrid(self, gx, gy, gz, src)
 *   - SphereArray_getOffset(self, dx, dy, dz, dest)
 *   - SphereArray_setOffset(self, dx, dy, dz, src)
 *   - SphereArray_forEach(self, callback, userData)
 *
 * Private Core Functions: (.c static)
 *   - SphereArray_index(gx, gy, gz, diameter)
 *
 * Getters:
 *   - SphereArray_radius(self)
 *   - SphereArray_diameter(self)
 *   - SphereArray_validVoxelCount(self)
 * ============================================================================
 */

SphereArray *SphereArray_createWithStride(int32_t radius, uint32_t elementClass, size_t stride) {
    if (radius < 0) {
        return nullptr;
    }
    int32_t diameter = 2 * radius + 1;
    size_t totalVoxels = (size_t) diameter * (size_t) diameter * (size_t) diameter;
    size_t dataBytes = totalVoxels * stride;

    SphereArray *self = (SphereArray*) Memory_alloc(TYPE_SPHERE_ARRAY, sizeof(SphereArray));
    if (self == nullptr) {
        return nullptr;
    }

    uint8_t *data = (uint8_t*) Memory_alloc(TYPE_BYTE_ARRAY, dataBytes);
    if (data == nullptr) {
        Memory_free(self);
        return nullptr;
    }
    memset(data, 0, dataBytes);

    (*self).collection.typeId = TYPE_SPHERE_ARRAY;
    (*self).collection.elementClass = elementClass;
    (*self).collection.stride = (uint32_t) stride;
    (*self).collection.capacity = (uint32_t) totalVoxels;
    (*self).collection.head = 0;
    (*self).collection.data = data;
    (*self).radius = radius;
    (*self).diameter = diameter;

    // Precalculate active count of voxels satisfying dx^2 + dy^2 + dz^2 <= r^2
    int64_t rSq = (int64_t) radius * (int64_t) radius;
    uint32_t active = 0;
    for (int32_t gz = 0; gz < diameter; gz++) {
        int64_t dz = (int64_t) gz - (int64_t) radius;
        int64_t dzSq = dz * dz;
        for (int32_t gy = 0; gy < diameter; gy++) {
            int64_t dy = (int64_t) gy - (int64_t) radius;
            int64_t dyzSq = dzSq + dy * dy;
            if (dyzSq > rSq) continue;
            for (int32_t gx = 0; gx < diameter; gx++) {
                int64_t dx = (int64_t) gx - (int64_t) radius;
                if (dyzSq + dx * dx <= rSq) {
                    active++;
                }
            }
        }
    }
    (*self).collection.activeCount = active;

    return self;
}

SphereArray *SphereArray_create(int32_t radius, uint32_t elementClass) {
    size_t stride = Stride_get(elementClass);
    if (stride == 0) {
        stride = 8;
    }
    return SphereArray_createWithStride(radius, elementClass, stride);
}

void SphereArray_free(SphereArray *self) {
    if (self == nullptr) {
        return;
    }
    if ((*self).collection.data != nullptr) {
        Memory_free((*self).collection.data);
        (*self).collection.data = nullptr;
    }
    Memory_free(self);
}

int32_t SphereArray_radius(const SphereArray *self) {
    if (self == nullptr) return 0;
    return (*self).radius;
}

int32_t SphereArray_diameter(const SphereArray *self) {
    if (self == nullptr) return 0;
    return (*self).diameter;
}

size_t SphereArray_validVoxelCount(const SphereArray *self) {
    if (self == nullptr) return 0;
    return (size_t) (*self).collection.activeCount;
}

int64_t SphereArray_distanceSquaredOffset(int32_t dx, int32_t dy, int32_t dz) {
    int64_t x = (int64_t) dx;
    int64_t y = (int64_t) dy;
    int64_t z = (int64_t) dz;
    return x * x + y * y + z * z;
}

int64_t SphereArray_distanceSquaredGrid(const SphereArray *self, int32_t gx, int32_t gy, int32_t gz) {
    if (self == nullptr) return -1;
    int64_t dx = (int64_t) gx - (int64_t) (*self).radius;
    int64_t dy = (int64_t) gy - (int64_t) (*self).radius;
    int64_t dz = (int64_t) gz - (int64_t) (*self).radius;
    return dx * dx + dy * dy + dz * dz;
}

bool SphereArray_containsOffset(const SphereArray *self, int32_t dx, int32_t dy, int32_t dz) {
    if (self == nullptr) return false;
    int32_t r = (*self).radius;
    if (dx < -r || dx > r || dy < -r || dy > r || dz < -r || dz > r) {
        return false;
    }
    int64_t distSq = SphereArray_distanceSquaredOffset(dx, dy, dz);
    int64_t rSq = (int64_t) r * (int64_t) r;
    return distSq <= rSq;
}

bool SphereArray_containsGrid(const SphereArray *self, int32_t gx, int32_t gy, int32_t gz) {
    if (self == nullptr) return false;
    int32_t d = (*self).diameter;
    if (gx < 0 || gx >= d || gy < 0 || gy >= d || gz < 0 || gz >= d) {
        return false;
    }
    int64_t distSq = SphereArray_distanceSquaredGrid(self, gx, gy, gz);
    int64_t rSq = (int64_t) (*self).radius * (int64_t) (*self).radius;
    return distSq <= rSq;
}

static inline size_t SphereArray_index(int32_t gx, int32_t gy, int32_t gz, int32_t diameter) {
    size_t d = (size_t) diameter;
    return ((size_t) gz * d * d) + ((size_t) gy * d) + (size_t) gx;
}

uint8_t *SphereArray_slotGrid(SphereArray *self, int32_t gx, int32_t gy, int32_t gz) {
    if (!SphereArray_containsGrid(self, gx, gy, gz)) {
        return nullptr;
    }
    size_t idx = SphereArray_index(gx, gy, gz, (*self).diameter);
    return (*self).collection.data + (idx * (size_t) (*self).collection.stride);
}

uint8_t *SphereArray_slotOffset(SphereArray *self, int32_t dx, int32_t dy, int32_t dz) {
    if (self == nullptr) return nullptr;
    int32_t gx = dx + (*self).radius;
    int32_t gy = dy + (*self).radius;
    int32_t gz = dz + (*self).radius;
    return SphereArray_slotGrid(self, gx, gy, gz);
}

bool SphereArray_getGrid(const SphereArray *self, int32_t gx, int32_t gy, int32_t gz, void *dest) {
    if (self == nullptr || dest == nullptr) return false;
    if (!SphereArray_containsGrid(self, gx, gy, gz)) {
        return false;
    }
    size_t idx = SphereArray_index(gx, gy, gz, (*self).diameter);
    size_t stride = (size_t) (*self).collection.stride;
    const uint8_t *src = (*self).collection.data + (idx * stride);
    memcpy(dest, src, stride);
    return true;
}

bool SphereArray_setGrid(SphereArray *self, int32_t gx, int32_t gy, int32_t gz, const void *src) {
    if (self == nullptr || src == nullptr) return false;
    if (!SphereArray_containsGrid(self, gx, gy, gz)) {
        return false;
    }
    size_t idx = SphereArray_index(gx, gy, gz, (*self).diameter);
    size_t stride = (size_t) (*self).collection.stride;
    uint8_t *dest = (*self).collection.data + (idx * stride);
    memcpy(dest, src, stride);
    return true;
}

bool SphereArray_getOffset(const SphereArray *self, int32_t dx, int32_t dy, int32_t dz, void *dest) {
    if (self == nullptr) return false;
    int32_t gx = dx + (*self).radius;
    int32_t gy = dy + (*self).radius;
    int32_t gz = dz + (*self).radius;
    return SphereArray_getGrid(self, gx, gy, gz, dest);
}

bool SphereArray_setOffset(SphereArray *self, int32_t dx, int32_t dy, int32_t dz, const void *src) {
    if (self == nullptr) return false;
    int32_t gx = dx + (*self).radius;
    int32_t gy = dy + (*self).radius;
    int32_t gz = dz + (*self).radius;
    return SphereArray_setGrid(self, gx, gy, gz, src);
}

void SphereArray_forEach(const SphereArray *self, SphereArrayVoxelFn callback, void *userData) {
    if (self == nullptr || callback == nullptr) return;
    int32_t r = (*self).radius;
    int32_t d = (*self).diameter;
    int64_t rSq = (int64_t) r * (int64_t) r;
    size_t stride = (size_t) (*self).collection.stride;

    for (int32_t gz = 0; gz < d; gz++) {
        int32_t dz = gz - r;
        int64_t dzSq = (int64_t) dz * (int64_t) dz;
        for (int32_t gy = 0; gy < d; gy++) {
            int32_t dy = gy - r;
            int64_t dyzSq = dzSq + ((int64_t) dy * (int64_t) dy);
            if (dyzSq > rSq) continue;
            for (int32_t gx = 0; gx < d; gx++) {
                int32_t dx = gx - r;
                if (dyzSq + ((int64_t) dx * (int64_t) dx) <= rSq) {
                    size_t idx = SphereArray_index(gx, gy, gz, d);
                    const uint8_t *voxel = (*self).collection.data + (idx * stride);
                    callback(gx, gy, gz, dx, dy, dz, voxel, userData);
                }
            }
        }
    }
}
