#include "algo/draft_sort.h"

#include <stdlib.h>

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: DraftSort
 * ============================================================================
 * Scaffolded sorting and spatial-indexing algorithms for darkbase table
 * ordering and relational spatial partitioning. Procedural: every function
 * operates on caller-owned buffers and owns no state, so there is no struct,
 * no allocation, and no teardown. DraftSort_partitionInt32 is a three-way
 * partition for quickselect on 32-bit keys; DraftSort_quicksortUint64 sorts
 * 64-bit row IDs in place; DraftSort_morton3D interleaves three 21-bit
 * coordinates into a 63-bit Z-order curve so spatially nearby voxels share
 * cache lines. Bounds: count elements for the array passes, 21 bits per axis
 * for the Morton interleave.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: DraftSort (algo/draft_sort.c — defined in algo/draft_sort.h)
 * LEVEL: L2 — Behavior (draft sorting and spatial Morton curve indexing)
 * ============================================================================
 * Scaffolded algorithms for darkbase table sorting and 3D spatial indexing.
 *
 * STRUCT FIELDS: none — procedural (operates on caller-owned int32/uint64
 * arrays and 21-bit XYZ Morton interleave)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - DraftSort_partitionInt32(array, count, pivot) : three-way partition
 *   - DraftSort_quicksortUint64(array, count)       : in-place 64-bit sort
 *   - DraftSort_morton3D(x, y, z)                   : 63-bit Z-order code
 *
 * Private Core Functions: (.c static)
 *   - compare_u64(a, b)  : qsort comparator for uint64
 *   - split_by_3(a)      : 21-bit bit-spread for Morton interleave
 * ============================================================================
 */

size_t DraftSort_partitionInt32(int32_t *array, size_t count, int32_t pivot) {
    if (!array || count == 0) return 0;
    size_t i = 0;
    for (size_t j = 0; j < count; j++) {
        if (array[j] < pivot) {
            int32_t tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
            i++;
        }
    }
    return i;
}

static int compare_u64(const void *a, const void *b) {
    uint64_t valA = *(const uint64_t*) a;
    uint64_t valB = *(const uint64_t*) b;
    if (valA < valB) return -1;
    if (valA > valB) return 1;
    return 0;
}

void DraftSort_quicksortUint64(uint64_t *array, size_t count) {
    if (!array || count <= 1) return;
    qsort(array, count, sizeof(uint64_t), compare_u64);
}

static inline uint64_t split_by_3(uint32_t a) {
    uint64_t x = a & 0x1fffff; // 21 bits
    x = (x | (x << 32)) & 0x1f00000000ffffULL;
    x = (x | (x << 16)) & 0x1f0000ff0000ffULL;
    x = (x | (x << 8))  & 0x100f00f00f00f00fULL;
    x = (x | (x << 4))  & 0x10c30c30c30c30c3ULL;
    x = (x | (x << 2))  & 0x1249249249249249ULL;
    return x;
}

uint64_t DraftSort_morton3D(uint32_t x, uint32_t y, uint32_t z) {
    return (split_by_3(x) << 2) | (split_by_3(y) << 1) | split_by_3(z);
}
