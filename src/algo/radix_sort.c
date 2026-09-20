#include "algo/radix_sort.h"

#include <stdlib.h>
#include <string.h>
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: RadixSort
 * ============================================================================
 * Least-Significant-Digit (LSD) Radix Sort with 256 counting buckets (8-bit
 * radix). Delivers predictable O(k*N) linear sorting time independent of
 * initial ordering, ideal for Morton spatial hashing, particle depth sorting,
 * and ECS entity batches. Procedural: no own state; temp buffers are drawn
 * from the Memory arena and freed before return, so the caller's arrays are
 * sorted in place with zero steady-state allocation.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: RadixSort (algo/radix_sort.c)
 * LEVEL: L1 — High-Throughput Sorting Core
 * ============================================================================
 * Least-Significant-Digit (LSD) Radix Sort with 256 counting buckets (8-bit radix).
 * Delivers predictable O(k*N) linear sorting time independent of initial ordering.
 *
 * STRUCT FIELDS: none — procedural (operates on caller-owned uint32_t/uint64_t
 * arrays and key/value pair arrays)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - RadixSort_u32(array, count)
 *   - RadixSort_u64(array, count)
 *   - RadixSort_pairsU64(keys, values, count)
 * ============================================================================
 */

void RadixSort_u32(uint32_t *array, size_t count) {
    if (array == nullptr || count <= 1) return;

    uint32_t *temp = (uint32_t*) Memory_alloc(TYPE_BYTE_ARRAY, count * sizeof(uint32_t));
    if (temp == nullptr) return;

    uint32_t *src = array;
    uint32_t *dst = temp;

    // 4 passes of 8 bits each
    for (int shift = 0; shift < 32; shift += 8) {
        size_t countBuckets[256] = {0};
        size_t offsetBuckets[256] = {0};

        // 1. Histogram
        for (size_t i = 0; i < count; i++) {
            uint8_t bucket = (uint8_t) ((src[i] >> shift) & 0xFF);
            countBuckets[bucket]++;
        }

        // 2. Prefix sums
        size_t runningSum = 0;
        for (int b = 0; b < 256; b++) {
            offsetBuckets[b] = runningSum;
            runningSum += countBuckets[b];
        }

        // 3. Scatter
        for (size_t i = 0; i < count; i++) {
            uint8_t bucket = (uint8_t) ((src[i] >> shift) & 0xFF);
            dst[offsetBuckets[bucket]++] = src[i];
        }

        // Swap buffers
        uint32_t *swap = src;
        src = dst;
        dst = swap;
    }

    // If result ended up in temp, copy back to array
    if (src != array) {
        memcpy(array, temp, count * sizeof(uint32_t));
    }

    Memory_free(temp);
}

void RadixSort_u64(uint64_t *array, size_t count) {
    if (array == nullptr || count <= 1) return;

    uint64_t *temp = (uint64_t*) Memory_alloc(TYPE_BYTE_ARRAY, count * sizeof(uint64_t));
    if (temp == nullptr) return;

    uint64_t *src = array;
    uint64_t *dst = temp;

    // 8 passes of 8 bits each
    for (int shift = 0; shift < 64; shift += 8) {
        size_t countBuckets[256] = {0};
        size_t offsetBuckets[256] = {0};

        for (size_t i = 0; i < count; i++) {
            uint8_t bucket = (uint8_t) ((src[i] >> shift) & 0xFF);
            countBuckets[bucket]++;
        }

        size_t runningSum = 0;
        for (int b = 0; b < 256; b++) {
            offsetBuckets[b] = runningSum;
            runningSum += countBuckets[b];
        }

        for (size_t i = 0; i < count; i++) {
            uint8_t bucket = (uint8_t) ((src[i] >> shift) & 0xFF);
            dst[offsetBuckets[bucket]++] = src[i];
        }

        uint64_t *swap = src;
        src = dst;
        dst = swap;
    }

    if (src != array) {
        memcpy(array, temp, count * sizeof(uint64_t));
    }

    Memory_free(temp);
}

void RadixSort_pairsU64(uint64_t *keys, void **values, size_t count) {
    if (keys == nullptr || values == nullptr || count <= 1) return;

    uint64_t *tempKeys = (uint64_t*) Memory_alloc(TYPE_BYTE_ARRAY, count * sizeof(uint64_t));
    void **tempVals = (void**) Memory_alloc(TYPE_BYTE_ARRAY, count * sizeof(void*));
    if (tempKeys == nullptr || tempVals == nullptr) {
        if (tempKeys != nullptr) Memory_free(tempKeys);
        if (tempVals != nullptr) Memory_free(tempVals);
        return;
    }

    uint64_t *srcKeys = keys;
    void **srcVals = values;
    uint64_t *dstKeys = tempKeys;
    void **dstVals = tempVals;

    for (int shift = 0; shift < 64; shift += 8) {
        size_t countBuckets[256] = {0};
        size_t offsetBuckets[256] = {0};

        for (size_t i = 0; i < count; i++) {
            uint8_t bucket = (uint8_t) ((srcKeys[i] >> shift) & 0xFF);
            countBuckets[bucket]++;
        }

        size_t runningSum = 0;
        for (int b = 0; b < 256; b++) {
            offsetBuckets[b] = runningSum;
            runningSum += countBuckets[b];
        }

        for (size_t i = 0; i < count; i++) {
            uint8_t bucket = (uint8_t) ((srcKeys[i] >> shift) & 0xFF);
            size_t destIdx = offsetBuckets[bucket]++;
            dstKeys[destIdx] = srcKeys[i];
            dstVals[destIdx] = srcVals[i];
        }

        uint64_t *swapK = srcKeys;
        srcKeys = dstKeys;
        dstKeys = swapK;

        void **swapV = srcVals;
        srcVals = dstVals;
        dstVals = swapV;
    }

    if (srcKeys != keys) {
        memcpy(keys, tempKeys, count * sizeof(uint64_t));
        memcpy(values, tempVals, count * sizeof(void*));
    }

    Memory_free(tempKeys);
    Memory_free(tempVals);
}
