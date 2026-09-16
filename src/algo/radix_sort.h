#ifndef ALGO_RADIX_SORT_H
#define ALGO_RADIX_SORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// algo/radix_sort.h — High-Performance Least-Significant-Digit (LSD) Radix Sort.
//
// Linear O(N) integer and key-value pair sorting using 8-bit (256-bucket) passes.
// Ideal for Morton spatial hashing, particle depth sorting, and ECS entity batches.

void RadixSort_u32(uint32_t *array, size_t count);
void RadixSort_u64(uint64_t *array, size_t count);

// Sort (key, value) pairs by 64-bit unsigned key in-place.
void RadixSort_pairsU64(uint64_t *keys, void **values, size_t count);

#endif
