#include "primitive/long_float.h"

#include <string.h>

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: LongFloat
 * ============================================================================
 * LongFloat primitive (Legacy: primitive/LongFloat.java): a procedural class
 * over a process-wide BitPool (16-byte elements — an int64 plus a float —
 * 1024 fixed slots) with a Memory arena fallback for large arrays.
 * LongFloat_alloc hands out pool slots typed ID_LONG_FLOAT;
 * LongFloat_allocArray switches to the arena when contiguity or size demands
 * it, and LongFloat_free routes to whichever owner holds the pointer.
 * LongFloat_allocWithValues packs the int64 and float halves of one payload.
 * LongFloat_init/LongFloat_shutdown bracket the pool's lifetime.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Long_float (primitive/long_float.c)
 * LEVEL: L2 — Behavior (primitive behavior API)
 * ============================================================================
 * LongFloat primitive (Legacy: primitive/LongFloat.java).
 *
 * STRUCT FIELDS: none — procedural (operates on BitPool/Memory blocks of long_float payloads)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - LongFloat_init(void)
 *
 * Core Functions:
 *   - LongFloat_shutdown(void)
 *   - LongFloat_alloc(void)
 *   - LongFloat_allocArray(count)
 *   - LongFloat_free(ptr)
 *   - LongFloat_compareAndSet(ptr, expected, value)
 *   - LongFloat_type(ptr)
 *   - LongFloat_length(ptr)
 *   - LongFloat_allocWithValues(v1, v2)
 *
 * Setters:
 *   - LongFloat_set(ptr, value)
 *
 * Getters:
 *   - LongFloat_get(ptr)
 * ============================================================================
 */


BitPool g_long_floatPool;

bool LongFloat_init(void) {
    return BitPool_init(&g_long_floatPool, 16, 1024);
}

void LongFloat_shutdown(void) {
    BitPool_shutdown(&g_long_floatPool);
}

void *LongFloat_alloc(void) {
    return BitPool_alloc(&g_long_floatPool, ID_LONG_FLOAT);
}

void *LongFloat_allocArray(size_t count) {
    if (count == 0)
        return nullptr;
    // Use Memory arena for arrays — BitPool is fixed 1024 slots, not for large contiguous
    size_t bytes = count * sizeof(int64_t);
    // For compound types, elem_size 16 already accounts for stride, but sizeof(int64_t) is placeholder
    // Use elem_size for true stride where c_type is int64 placeholder
    if (count > 1 && 16 != sizeof(int64_t))
        bytes = count * 16;
    return Memory_alloc(Type_make(PROJ_VEXSPOKE, FORM_ARRAY, ID_LONG_FLOAT), bytes);
}

void LongFloat_free(void *ptr) {
    if (!ptr)
        return;
    if (BitPool_contains(&g_long_floatPool, ptr)) BitPool_free(&g_long_floatPool, ptr);
    else Memory_free(ptr);
}

int64_t LongFloat_get(void *ptr) {
    if (!ptr)
        return (int64_t) 0;
    return *(int64_t*) ptr;
}

void LongFloat_set(void *ptr, int64_t value) {
    if (!ptr)
        return;
    *(int64_t*) ptr = value;
}

bool LongFloat_compareAndSet(void *ptr, int64_t expected, int64_t value) {
    if (!ptr)
        return false;
    // fallback to simple CAS via __atomic_compare_exchange
    return __atomic_compare_exchange_n((int64_t*) ptr, &expected, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

uint64_t LongFloat_type(void *ptr) {
    if (!ptr)
        return 0;
    if (BitPool_contains(&g_long_floatPool, ptr)) return BitPool_type(&g_long_floatPool, ptr);
    return Memory_type(ptr);
}

size_t LongFloat_length(void *ptr) {
    if (!ptr)
        return 0;
    if (BitPool_contains(&g_long_floatPool, ptr)) return BitPool_length(&g_long_floatPool, ptr);
    return Memory_length(ptr);
}

void *LongFloat_allocWithValues(int64_t v1, float v2) {
    void *ptr = LongFloat_alloc();
    if (!ptr) return nullptr;
    *(int64_t*) ptr = v1;
    *(float*) ((uint8_t*) ptr + sizeof(int64_t)) = v2;
    return ptr;
}
