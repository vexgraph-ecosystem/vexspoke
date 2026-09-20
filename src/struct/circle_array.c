#include "struct/circle_array.h"

#include <string.h>
#include "nio/mem.h"
#include "oop/stride.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: CircleArray
 * ============================================================================
 * 2D circular matrix evaluated via the Pythagorean Theorem. Encapsulates a
 * square bounding grid of size (2*radius + 1)^2 where only cells whose
 * Euclidean distance from the center satisfies dx^2 + dy^2 <= radius^2 are
 * valid and accessible. Arena-allocated: the CircleArray embeds a Collection
 * header and owns a flat byte data buffer with per-element stride, so cell
 * access is index math into one contiguous block with zero pointer chasing.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: CircleArray (struct/circle_array.c)
 * LEVEL: L2 — Off-Heap Spatial Container
 * ============================================================================
 * 2D Circular Matrix evaluated via the Pythagorean Theorem.
 *
 * Encapsulates a square bounding grid of size (2*radius + 1)^2 where only cells
 * whose Euclidean distance from the center satisfies dx^2 + dy^2 <= radius^2
 * are considered valid and accessible.
 *
 * STRUCT FIELDS (Mirroring struct/circle_array.h + struct/collection.h):
 * ----------------------------------------------------------------------------
 *   CircleArray {
 *     Collection collection; // base collection header (typeId, activeCount, etc.)
 *     int32_t    radius;     // radius in cells
 *     int32_t    diameter;   // 2 * radius + 1
 *   }
 *   Collection {
 *     uint64_t typeId;       // mirror of the block-header type (for debug)
 *     uint32_t activeCount;  // number of live elements
 *     uint32_t elementClass; // class of elements (Map: key class)
 *     uint32_t stride;       // bytes per element (Map: val class)
 *     uint32_t capacity;     // element capacity (or slot capacity)
 *     uint32_t head;         // circular head index (Deque/Queue); else 0
 *     uint8_t *data;         // element / slot buffer
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - CircleArray_create(radius, elementClass)
 *   - CircleArray_createWithStride(radius, elementClass, stride)
 *
 * Public Core Functions: (.h)
 *   - CircleArray_free(self)
 *   - CircleArray_radius(self)
 *   - CircleArray_diameter(self)
 *   - CircleArray_validCellCount(self)
 *   - CircleArray_containsGrid(self, gridX, gridY)
 *   - CircleArray_containsOffset(self, dx, dy)
 *   - CircleArray_distanceSquaredGrid(self, gridX, gridY)
 *   - CircleArray_distanceSquaredOffset(dx, dy)
 *   - CircleArray_getGrid(self, gridX, gridY, dest)
 *   - CircleArray_setGrid(self, gridX, gridY, src)
 *   - CircleArray_getOffset(self, dx, dy, dest)
 *   - CircleArray_setOffset(self, dx, dy, src)
 *   - CircleArray_slotGrid(self, gridX, gridY)
 *   - CircleArray_slotOffset(self, dx, dy)
 *   - CircleArray_forEach(self, callback, userData)
 * ============================================================================
 */

CircleArray *CircleArray_createWithStride(int32_t radius, uint32_t elementClass, size_t stride) {
    if (radius < 0) {
        return nullptr;
    }
    int32_t diameter = 2 * radius + 1;
    size_t totalCells = (size_t) diameter * (size_t) diameter;
    size_t dataBytes = totalCells * stride;

    CircleArray *self = (CircleArray*) Memory_alloc(TYPE_CIRCLE_ARRAY, sizeof(CircleArray));
    if (self == nullptr) {
        return nullptr;
    }

    uint8_t *data = (uint8_t*) Memory_alloc(TYPE_BYTE_ARRAY, dataBytes);
    if (data == nullptr) {
        Memory_free(self);
        return nullptr;
    }
    memset(data, 0, dataBytes);

    (*self).collection.typeId = TYPE_CIRCLE_ARRAY;
    (*self).collection.elementClass = elementClass;
    (*self).collection.stride = (uint32_t) stride;
    (*self).collection.capacity = (uint32_t) totalCells;
    (*self).collection.head = 0;
    (*self).collection.data = data;
    (*self).radius = radius;
    (*self).diameter = diameter;

    // Calculate active count: number of valid cells where Pythagorean inequality holds
    int64_t rSq = (int64_t) radius * (int64_t) radius;
    uint32_t active = 0;
    for (int32_t gy = 0; gy < diameter; gy++) {
        int64_t dy = (int64_t) gy - (int64_t) radius;
        for (int32_t gx = 0; gx < diameter; gx++) {
            int64_t dx = (int64_t) gx - (int64_t) radius;
            if (dx * dx + dy * dy <= rSq) {
                active++;
            }
        }
    }
    (*self).collection.activeCount = active;

    return self;
}

CircleArray *CircleArray_create(int32_t radius, uint32_t elementClass) {
    size_t stride = Stride_get(elementClass);
    if (stride == 0) {
        stride = 8;
    }
    return CircleArray_createWithStride(radius, elementClass, stride);
}

void CircleArray_free(CircleArray *self) {
    if (self == nullptr) {
        return;
    }
    if ((*self).collection.data != nullptr) {
        Memory_free((*self).collection.data);
        (*self).collection.data = nullptr;
    }
    Memory_free(self);
}

int32_t CircleArray_radius(const CircleArray *self) {
    if (self == nullptr) return 0;
    return (*self).radius;
}

int32_t CircleArray_diameter(const CircleArray *self) {
    if (self == nullptr) return 0;
    return (*self).diameter;
}

size_t CircleArray_validCellCount(const CircleArray *self) {
    if (self == nullptr) return 0;
    return (size_t) (*self).collection.activeCount;
}

int64_t CircleArray_distanceSquaredOffset(int32_t dx, int32_t dy) {
    int64_t x = (int64_t) dx;
    int64_t y = (int64_t) dy;
    return x * x + y * y;
}

int64_t CircleArray_distanceSquaredGrid(const CircleArray *self, int32_t gridX, int32_t gridY) {
    if (self == nullptr) return -1;
    int64_t dx = (int64_t) gridX - (int64_t) (*self).radius;
    int64_t dy = (int64_t) gridY - (int64_t) (*self).radius;
    return dx * dx + dy * dy;
}

bool CircleArray_containsOffset(const CircleArray *self, int32_t dx, int32_t dy) {
    if (self == nullptr) return false;
    int32_t r = (*self).radius;
    if (dx < -r || dx > r || dy < -r || dy > r) {
        return false;
    }
    int64_t distSq = CircleArray_distanceSquaredOffset(dx, dy);
    int64_t rSq = (int64_t) r * (int64_t) r;
    return distSq <= rSq;
}

bool CircleArray_containsGrid(const CircleArray *self, int32_t gridX, int32_t gridY) {
    if (self == nullptr) return false;
    int32_t d = (*self).diameter;
    if (gridX < 0 || gridX >= d || gridY < 0 || gridY >= d) {
        return false;
    }
    int64_t distSq = CircleArray_distanceSquaredGrid(self, gridX, gridY);
    int64_t rSq = (int64_t) (*self).radius * (int64_t) (*self).radius;
    return distSq <= rSq;
}

uint8_t *CircleArray_slotGrid(CircleArray *self, int32_t gridX, int32_t gridY) {
    if (!CircleArray_containsGrid(self, gridX, gridY)) {
        return nullptr;
    }
    size_t index = (size_t) gridY * (size_t) (*self).diameter + (size_t) gridX;
    return (*self).collection.data + (index * (size_t) (*self).collection.stride);
}

uint8_t *CircleArray_slotOffset(CircleArray *self, int32_t dx, int32_t dy) {
    if (self == nullptr) return nullptr;
    int32_t gridX = dx + (*self).radius;
    int32_t gridY = dy + (*self).radius;
    return CircleArray_slotGrid(self, gridX, gridY);
}

bool CircleArray_getGrid(const CircleArray *self, int32_t gridX, int32_t gridY, void *dest) {
    if (self == nullptr || dest == nullptr) return false;
    if (!CircleArray_containsGrid(self, gridX, gridY)) {
        return false;
    }
    size_t index = (size_t) gridY * (size_t) (*self).diameter + (size_t) gridX;
    size_t stride = (size_t) (*self).collection.stride;
    const uint8_t *src = (*self).collection.data + (index * stride);
    memcpy(dest, src, stride);
    return true;
}

bool CircleArray_setGrid(CircleArray *self, int32_t gridX, int32_t gridY, const void *src) {
    if (self == nullptr || src == nullptr) return false;
    if (!CircleArray_containsGrid(self, gridX, gridY)) {
        return false;
    }
    size_t index = (size_t) gridY * (size_t) (*self).diameter + (size_t) gridX;
    size_t stride = (size_t) (*self).collection.stride;
    uint8_t *dest = (*self).collection.data + (index * stride);
    memcpy(dest, src, stride);
    return true;
}

bool CircleArray_getOffset(const CircleArray *self, int32_t dx, int32_t dy, void *dest) {
    if (self == nullptr) return false;
    int32_t gridX = dx + (*self).radius;
    int32_t gridY = dy + (*self).radius;
    return CircleArray_getGrid(self, gridX, gridY, dest);
}

bool CircleArray_setOffset(CircleArray *self, int32_t dx, int32_t dy, const void *src) {
    if (self == nullptr) return false;
    int32_t gridX = dx + (*self).radius;
    int32_t gridY = dy + (*self).radius;
    return CircleArray_setGrid(self, gridX, gridY, src);
}

void CircleArray_forEach(const CircleArray *self, CircleArrayCellFn callback, void *userData) {
    if (self == nullptr || callback == nullptr) return;
    int32_t r = (*self).radius;
    int32_t d = (*self).diameter;
    int64_t rSq = (int64_t) r * (int64_t) r;
    size_t stride = (size_t) (*self).collection.stride;

    for (int32_t gy = 0; gy < d; gy++) {
        int32_t dy = gy - r;
        int64_t dySq = (int64_t) dy * (int64_t) dy;
        for (int32_t gx = 0; gx < d; gx++) {
            int32_t dx = gx - r;
            if ((int64_t) dx * (int64_t) dx + dySq <= rSq) {
                size_t index = (size_t) gy * (size_t) d + (size_t) gx;
                const uint8_t *element = (*self).collection.data + (index * stride);
                callback(gx, gy, dx, dy, element, userData);
            }
        }
    }
}
