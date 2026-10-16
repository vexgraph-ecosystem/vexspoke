#include "algo/spatial.h"

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Spatial
 * ============================================================================
 * Stateless spatial indexing helpers. Spatial_morton3D interleaves the low
 * 21 bits of each axis into one 63-bit Morton (Z-order) code. Coordinates are
 * caller-owned scalar inputs; this module allocates no storage.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Spatial (algo/spatial.c)
 * ============================================================================
 * Compact spatial indexing helpers.
 *
 * STRUCT FIELDS: none — procedural coordinate transforms
 *
 * FUNCTION REGISTRY:
 *   - Spatial_morton3D(x, y, z) : interleave three 21-bit axes
 * ============================================================================
 */

static uint64_t spreadAxisBits(uint32_t coordinate) {
    uint64_t value = coordinate & 0x1FFFFFu;
    value = (value | (value << 32u)) & 0x1F00000000FFFFull;
    value = (value | (value << 16u)) & 0x1F0000FF0000FFull;
    value = (value | (value << 8u)) & 0x100F00F00F00F00Full;
    value = (value | (value << 4u)) & 0x10C30C30C30C30C3ull;
    value = (value | (value << 2u)) & 0x1249249249249249ull;
    return value;
}

uint64_t Spatial_morton3D(uint32_t x, uint32_t y, uint32_t z) {
    return (spreadAxisBits(x) << 2u)
        | (spreadAxisBits(y) << 1u)
        | spreadAxisBits(z);
}
