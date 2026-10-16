#include "algo/sort.h"

#include <string.h>

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Sort
 * ============================================================================
 * Stateless sorting algorithms over caller-owned arrays. Comparison sorts
 * use the caller's comparator and never allocate. Merge sort receives its
 * scratch space from the caller. Unsigned radix sorts use temporary storage
 * from the Relational Engine Memory ABI and report allocation failure without
 * partially reordering the input.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Sort (algo/sort.c)
 * ============================================================================
 * In-place comparison sorts, stable merge sort, integer radix sorts and a
 * signed 32-bit partition helper.
 *
 * STRUCT FIELDS: none — procedural algorithms over caller-owned arrays
 *
 * PRIVATE HELPERS:
 *   swapElements(left, right, size)       // swap fixed-width records
 *   insertionSortRange(values, begin, end, size, compare) // stable small sort
 *   siftDown(values, root, count, size, compare) // restore max heap
 *   heapSortRange(values, begin, end, size, compare) // in-place heap sort
 *   partitionRange(values, begin, end, size, compare) // pivot partition
 *   quickSortRange(values, begin, end, size, compare, depth) // introspective quicksort
 *   unsignedKeyAt(values, index, size)    // read a u32/u64 radix key
 *   radixSortUnsigned(values, count, size) // stable byte-wise radix passes
 *
 * FUNCTION REGISTRY:
 *   Quick sort, insertion sort, selection sort, bubble sort, heap sort,
 *   merge sort, int32 partition, uint32/uint64 radix sort and paired-u64 radix.
 * ============================================================================
 */

static unsigned char *elementAt(void *values, size_t index, size_t elementSize) {
    return (unsigned char*) values + index * elementSize;
}

static const unsigned char *constElementAt(const void *values, size_t index,
                                            size_t elementSize) {
    return (const unsigned char*) values + index * elementSize;
}

static void swapElements(void *left, void *right, size_t elementSize) {
    if (left == right)
        return;
    unsigned char *a = left;
    unsigned char *b = right;
    for (size_t i = 0u; i < elementSize; ++i) {
        unsigned char byte = a[i];
        a[i] = b[i];
        b[i] = byte;
    }
}

static bool validComparisonRange(const void *values, size_t count,
                                 size_t elementSize, SortCompareFn compare) {
    if (count == 0u)
        return true;
    return values != nullptr && elementSize > 0u && compare != nullptr
        && count <= SIZE_MAX / elementSize;
}

static void insertionSortRange(void *values, size_t begin, size_t end,
                               size_t elementSize, SortCompareFn compare) {
    for (size_t i = begin + (begin < end); i < end; ++i) {
        size_t current = i;
        while (current > begin
               && compare(constElementAt(values, current, elementSize),
                          constElementAt(values, current - 1u, elementSize)) < 0) {
            swapElements(elementAt(values, current, elementSize),
                         elementAt(values, current - 1u, elementSize), elementSize);
            --current;
        }
    }
}

bool Sort_insertionSort(void *values, size_t count, size_t elementSize,
                        SortCompareFn compare) {
    if (!validComparisonRange(values, count, elementSize, compare))
        return false;
    insertionSortRange(values, 0u, count, elementSize, compare);
    return true;
}

bool Sort_selectionSort(void *values, size_t count, size_t elementSize,
                        SortCompareFn compare) {
    if (!validComparisonRange(values, count, elementSize, compare))
        return false;
    for (size_t first = 0u; first < count; ++first) {
        size_t minimum = first;
        for (size_t candidate = first + 1u; candidate < count; ++candidate) {
            if (compare(constElementAt(values, candidate, elementSize),
                        constElementAt(values, minimum, elementSize)) < 0)
                minimum = candidate;
        }
        swapElements(elementAt(values, first, elementSize),
                     elementAt(values, minimum, elementSize), elementSize);
    }
    return true;
}

bool Sort_bubbleSort(void *values, size_t count, size_t elementSize,
                     SortCompareFn compare) {
    if (!validComparisonRange(values, count, elementSize, compare))
        return false;
    for (size_t unsorted = count; unsorted > 1u; --unsorted) {
        bool changed = false;
        for (size_t i = 1u; i < unsorted; ++i) {
            if (compare(constElementAt(values, i, elementSize),
                        constElementAt(values, i - 1u, elementSize)) < 0) {
                swapElements(elementAt(values, i, elementSize),
                             elementAt(values, i - 1u, elementSize), elementSize);
                changed = true;
            }
        }
        if (!changed)
            break;
    }
    return true;
}

static void siftDown(void *values, size_t root, size_t count,
                     size_t elementSize, SortCompareFn compare) {
    while (root < count / 2u) {
        size_t child = root * 2u + 1u;
        if (child + 1u < count
            && compare(constElementAt(values, child, elementSize),
                       constElementAt(values, child + 1u, elementSize)) < 0)
            ++child;
        if (compare(constElementAt(values, root, elementSize),
                    constElementAt(values, child, elementSize)) >= 0)
            return;
        swapElements(elementAt(values, root, elementSize),
                     elementAt(values, child, elementSize), elementSize);
        root = child;
    }
}

static void heapSortRange(void *values, size_t begin, size_t end,
                          size_t elementSize, SortCompareFn compare) {
    size_t count = end - begin;
    if (count < 2u)
        return;
    unsigned char *range = elementAt(values, begin, elementSize);
    for (size_t parent = count / 2u; parent > 0u; --parent)
        siftDown(range, parent - 1u, count, elementSize, compare);
    for (size_t remaining = count; remaining > 1u; --remaining) {
        swapElements(elementAt(range, 0u, elementSize),
                     elementAt(range, remaining - 1u, elementSize), elementSize);
        siftDown(range, 0u, remaining - 1u, elementSize, compare);
    }
}

bool Sort_heapSort(void *values, size_t count, size_t elementSize,
                   SortCompareFn compare) {
    if (!validComparisonRange(values, count, elementSize, compare))
        return false;
    heapSortRange(values, 0u, count, elementSize, compare);
    return true;
}

static size_t partitionRange(void *values, size_t begin, size_t end,
                             size_t elementSize, SortCompareFn compare) {
    size_t last = end - 1u;
    size_t middle = begin + (last - begin) / 2u;
    if (compare(constElementAt(values, begin, elementSize),
                constElementAt(values, middle, elementSize)) > 0)
        swapElements(elementAt(values, begin, elementSize),
                     elementAt(values, middle, elementSize), elementSize);
    if (compare(constElementAt(values, middle, elementSize),
                constElementAt(values, last, elementSize)) > 0)
        swapElements(elementAt(values, middle, elementSize),
                     elementAt(values, last, elementSize), elementSize);
    if (compare(constElementAt(values, begin, elementSize),
                constElementAt(values, middle, elementSize)) > 0)
        swapElements(elementAt(values, begin, elementSize),
                     elementAt(values, middle, elementSize), elementSize);
    swapElements(elementAt(values, middle, elementSize),
                 elementAt(values, last, elementSize), elementSize);

    size_t boundary = begin;
    for (size_t candidate = begin; candidate < last; ++candidate) {
        if (compare(constElementAt(values, candidate, elementSize),
                    constElementAt(values, last, elementSize)) < 0) {
            swapElements(elementAt(values, boundary, elementSize),
                         elementAt(values, candidate, elementSize), elementSize);
            ++boundary;
        }
    }
    swapElements(elementAt(values, boundary, elementSize),
                 elementAt(values, last, elementSize), elementSize);
    return boundary;
}

static void quickSortRange(void *values, size_t begin, size_t end,
                           size_t elementSize, SortCompareFn compare,
                           size_t depthLimit) {
    while (end - begin > 16u) {
        if (depthLimit == 0u) {
            heapSortRange(values, begin, end, elementSize, compare);
            return;
        }
        --depthLimit;
        size_t pivot = partitionRange(values, begin, end, elementSize, compare);
        size_t leftCount = pivot - begin;
        size_t rightBegin = pivot + 1u;
        size_t rightCount = end - rightBegin;
        if (leftCount < rightCount) {
            quickSortRange(values, begin, pivot, elementSize, compare, depthLimit);
            begin = rightBegin;
        } else {
            quickSortRange(values, rightBegin, end, elementSize, compare, depthLimit);
            end = pivot;
        }
    }
    insertionSortRange(values, begin, end, elementSize, compare);
}

bool Sort_quickSort(void *values, size_t count, size_t elementSize,
                    SortCompareFn compare) {
    if (!validComparisonRange(values, count, elementSize, compare))
        return false;
    size_t depthLimit = 0u;
    for (size_t remaining = count; remaining > 1u; remaining >>= 1u)
        depthLimit += 2u;
    quickSortRange(values, 0u, count, elementSize, compare, depthLimit);
    return true;
}

bool Sort_mergeSort(void *values, size_t count, size_t elementSize,
                    SortCompareFn compare, void *scratch) {
    if (!validComparisonRange(values, count, elementSize, compare))
        return false;
    if (count <= 1u)
        return true;
    if (scratch == nullptr)
        return false;

    size_t byteCount = count * elementSize;
    uintptr_t valuesStart = (uintptr_t) values;
    uintptr_t scratchStart = (uintptr_t) scratch;
    if (valuesStart > UINTPTR_MAX - byteCount
        || scratchStart > UINTPTR_MAX - byteCount
        || (valuesStart < scratchStart + byteCount
            && scratchStart < valuesStart + byteCount))
        return false;
    void *source = values;
    void *destination = scratch;
    for (size_t run = 1u; run < count;) {
        size_t left = 0u;
        while (left < count) {
            size_t middle = left + (run < count - left ? run : count - left);
            size_t right = middle + (run < count - middle ? run : count - middle);
            size_t a = left;
            size_t b = middle;
            size_t out = left;
            while (a < middle && b < right) {
                if (compare(constElementAt(source, a, elementSize),
                            constElementAt(source, b, elementSize)) <= 0) {
                    memcpy(elementAt(destination, out++, elementSize),
                           constElementAt(source, a++, elementSize), elementSize);
                } else {
                    memcpy(elementAt(destination, out++, elementSize),
                           constElementAt(source, b++, elementSize), elementSize);
                }
            }
            while (a < middle)
                memcpy(elementAt(destination, out++, elementSize),
                       constElementAt(source, a++, elementSize), elementSize);
            while (b < right)
                memcpy(elementAt(destination, out++, elementSize),
                       constElementAt(source, b++, elementSize), elementSize);
            left = right;
        }
        void *swap = source;
        source = destination;
        destination = swap;
        run = run > count / 2u ? count : run * 2u;
    }
    if (source != values)
        memcpy(values, source, byteCount);
    return true;
}

size_t Sort_partitionInt32(int32_t *values, size_t count, int32_t pivot) {
    if (values == nullptr || count == 0u)
        return 0u;
    size_t boundary = 0u;
    for (size_t i = 0u; i < count; ++i) {
        if (values[i] < pivot) {
            int32_t value = values[boundary];
            values[boundary] = values[i];
            values[i] = value;
            ++boundary;
        }
    }
    return boundary;
}

static uint64_t unsignedKeyAt(const void *values, size_t index,
                              size_t keySize) {
    if (keySize == sizeof(uint32_t))
        return ((const uint32_t*) values)[index];
    return ((const uint64_t*) values)[index];
}

static bool radixSortUnsigned(void *values, size_t count, size_t keySize) {
    if (count <= 1u)
        return true;
    if (values == nullptr || count > SIZE_MAX / keySize)
        return false;
    size_t byteCount = count * keySize;
    void *temporary = Memory_alloc(TYPE_BYTE_ARRAY, byteCount);
    if (temporary == nullptr)
        return false;

    void *source = values;
    void *destination = temporary;
    unsigned passes = (unsigned) keySize;
    for (unsigned pass = 0u; pass < passes; ++pass) {
        size_t bucketCounts[256] = {0};
        size_t bucketOffsets[256] = {0};
        unsigned shift = pass * 8u;
        for (size_t i = 0u; i < count; ++i) {
            uint8_t bucket = (uint8_t) ((unsignedKeyAt(source, i, keySize) >> shift) & 0xFFu);
            ++bucketCounts[bucket];
        }
        size_t offset = 0u;
        for (size_t bucket = 0u; bucket < 256u; ++bucket) {
            bucketOffsets[bucket] = offset;
            offset += bucketCounts[bucket];
        }
        for (size_t i = 0u; i < count; ++i) {
            uint64_t key = unsignedKeyAt(source, i, keySize);
            uint8_t bucket = (uint8_t) ((key >> shift) & 0xFFu);
            size_t target = bucketOffsets[bucket]++;
            memcpy(elementAt(destination, target, keySize),
                   constElementAt(source, i, keySize), keySize);
        }
        void *swap = source;
        source = destination;
        destination = swap;
    }
    if (source != values)
        memcpy(values, source, byteCount);
    Memory_free(temporary);
    return true;
}

bool Sort_radixSortUint32(uint32_t *values, size_t count) {
    return radixSortUnsigned(values, count, sizeof(uint32_t));
}

bool Sort_radixSortUint64(uint64_t *values, size_t count) {
    return radixSortUnsigned(values, count, sizeof(uint64_t));
}

bool Sort_radixSortPairsUint64(uint64_t *keys, void **values, size_t count) {
    if (count <= 1u)
        return true;
    if (keys == nullptr || values == nullptr
        || count > SIZE_MAX / sizeof(uint64_t)
        || count > SIZE_MAX / sizeof(void*))
        return false;
    size_t keyBytes = count * sizeof(uint64_t);
    size_t valueBytes = count * sizeof(void*);
    uint64_t *temporaryKeys = (uint64_t*) Memory_alloc(TYPE_BYTE_ARRAY, keyBytes);
    void **temporaryValues = (void**) Memory_alloc(TYPE_BYTE_ARRAY, valueBytes);
    if (temporaryKeys == nullptr || temporaryValues == nullptr) {
        if (temporaryKeys != nullptr)
            Memory_free(temporaryKeys);
        if (temporaryValues != nullptr)
            Memory_free(temporaryValues);
        return false;
    }

    uint64_t *sourceKeys = keys;
    uint64_t *destinationKeys = temporaryKeys;
    void **sourceValues = values;
    void **destinationValues = temporaryValues;
    for (unsigned pass = 0u; pass < 8u; ++pass) {
        size_t bucketCounts[256] = {0};
        size_t bucketOffsets[256] = {0};
        unsigned shift = pass * 8u;
        for (size_t i = 0u; i < count; ++i) {
            uint8_t bucket = (uint8_t) ((sourceKeys[i] >> shift) & 0xFFu);
            ++bucketCounts[bucket];
        }
        size_t offset = 0u;
        for (size_t bucket = 0u; bucket < 256u; ++bucket) {
            bucketOffsets[bucket] = offset;
            offset += bucketCounts[bucket];
        }
        for (size_t i = 0u; i < count; ++i) {
            uint8_t bucket = (uint8_t) ((sourceKeys[i] >> shift) & 0xFFu);
            size_t target = bucketOffsets[bucket]++;
            destinationKeys[target] = sourceKeys[i];
            destinationValues[target] = sourceValues[i];
        }
        uint64_t *keySwap = sourceKeys;
        sourceKeys = destinationKeys;
        destinationKeys = keySwap;
        void **valueSwap = sourceValues;
        sourceValues = destinationValues;
        destinationValues = valueSwap;
    }
    if (sourceKeys != keys) {
        memcpy(keys, sourceKeys, keyBytes);
        memcpy(values, sourceValues, valueBytes);
    }
    Memory_free(temporaryKeys);
    Memory_free(temporaryValues);
    return true;
}
