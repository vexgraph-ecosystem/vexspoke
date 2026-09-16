#ifndef ALGO_DRAFT_SORT_H
#define ALGO_DRAFT_SORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "annotation/draft.h"
#include "annotation/intention.h"

// algo/draft_sort.h — Draft Sorting & Spatial Partitioning Algorithms.
//
// Single Class Per File Law: DraftSort.
//
// Draft algorithms to be shaped for high-performance darkbase table ordering
// and relational spatial partitioning.

// Three-way quickselect / partition on 32-bit integer keys.
size_t DraftSort_partitionInt32(int32_t *array, size_t count, int32_t pivot);

// In-place dual-pivot quicksort for 64-bit row IDs.
void DraftSort_quicksortUint64(uint64_t *array, size_t count);

// Spatial Morton code (Z-order curve) 3D interleave for fast spatial indexing.
uint64_t DraftSort_morton3D(uint32_t x, uint32_t y, uint32_t z);

#endif
