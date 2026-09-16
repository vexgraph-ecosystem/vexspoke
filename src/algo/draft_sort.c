#include "algo/draft_sort.h"

#include <stdlib.h>

#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: DraftSort (algo/draft_sort.c — defined in algo/draft_sort.h)
 * LEVEL: L2 — Behavior (draft sorting and spatial Morton curve indexing)
 * ============================================================================
 * Scaffolded algorithms for darkbase table sorting and 3D spatial indexing.
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
