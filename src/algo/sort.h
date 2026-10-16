#ifndef ALGO_SORT_H
#define ALGO_SORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// algo/sort.h — in-place comparison sorts and unsigned radix sorts.
//
// Comparison sorts operate on caller-owned arrays and require a comparator
// whose ordering remains consistent for the duration of the call. They do not
// allocate. Sort_mergeSort requires a non-overlapping caller-owned scratch buffer of
// count * elementSize bytes. Radix sorts use the Relational Engine Memory ABI.

typedef int (*SortCompareFn)(const void *left, const void *right);

bool Sort_quickSort(void *values, size_t count, size_t elementSize,
                    SortCompareFn compare);
bool Sort_insertionSort(void *values, size_t count, size_t elementSize,
                        SortCompareFn compare);
bool Sort_selectionSort(void *values, size_t count, size_t elementSize,
                        SortCompareFn compare);
bool Sort_bubbleSort(void *values, size_t count, size_t elementSize,
                     SortCompareFn compare);
bool Sort_heapSort(void *values, size_t count, size_t elementSize,
                   SortCompareFn compare);
bool Sort_mergeSort(void *values, size_t count, size_t elementSize,
                    SortCompareFn compare, void *scratch);

// Two-way partition: [0, boundary) is below pivot; the remainder is greater
// than or equal to pivot. This operation is not stable. Returns zero for
// nullptr or an empty range.
size_t Sort_partitionInt32(int32_t *values, size_t count, int32_t pivot);

// Stable unsigned LSD radix sorts. Return false for invalid input or allocation
// failure; on allocation failure the input arrays remain unchanged.
bool Sort_radixSortUint32(uint32_t *values, size_t count);
bool Sort_radixSortUint64(uint64_t *values, size_t count);
bool Sort_radixSortPairsUint64(uint64_t *keys, void **values, size_t count);

#endif
