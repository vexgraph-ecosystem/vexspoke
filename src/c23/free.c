#include "c23/free.h"
#undef free // We need the real free() for Memory_free later if we call it directly, though we actually call Memory_free here.

#include "nio/mem.h"
#include "oop/type.h"
#include "objects/probable.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Free
 * ============================================================================
 * The relational destructor dispatcher: c23_free routes a pointer's runtime
 * Memory_type through a fixed 64-entry dynamic destructor table (registered
 * via Destructor_register, first-match wins), then through the built-in type
 * cases, and finally reclaims the raw block with Memory_free. It is the cold
 * teardown seam for every arena-backed object, so it must never allocate,
 * block, or recurse. Registration is idempotent: re-registering a typeId
 * replaces the handler in place.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Free (c23/free.c)
 * LEVEL: L2 — Behavior (relational runtime behavior)
 * ============================================================================
 * The Relational Destructor Dispatcher.
 *
 * STRUCT FIELDS: none — procedural (operates on generic Memory blocks via runtime type dispatch)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - c23_free(ptr)
 * ============================================================================
 */


typedef struct DestructorEntry {
    uint64_t     typeId;
    DestructorFn fn;
} DestructorEntry;

#define MAX_CUSTOM_DESTRUCTORS 64
static DestructorEntry s_destructors[MAX_CUSTOM_DESTRUCTORS];
static uint32_t s_destructorCount = 0;

void Destructor_register(uint64_t typeId, DestructorFn fn) {
    if (fn == nullptr || typeId == 0) return;
    // Check if already registered to update
    for (uint32_t i = 0; i < s_destructorCount; i++) {
        if (s_destructors[i].typeId == typeId) {
            s_destructors[i].fn = fn;
            return;
        }
    }
    if (s_destructorCount < MAX_CUSTOM_DESTRUCTORS) {
        s_destructors[s_destructorCount].typeId = typeId;
        s_destructors[s_destructorCount].fn = fn;
        s_destructorCount++;
    }
}

DestructorFn Destructor_lookup(uint64_t typeId) {
    for (uint32_t i = 0; i < s_destructorCount; i++) {
        if (s_destructors[i].typeId == typeId) {
            return s_destructors[i].fn;
        }
    }
    return nullptr;
}

void c23_free(void *ptr) {
    if (!ptr) return;

    uint64_t typeId = Memory_type(ptr);

    // 1. Dynamic destructor table lookup
    for (uint32_t i = 0; i < s_destructorCount; i++) {
        if (s_destructors[i].typeId == typeId) {
            if (s_destructors[i].fn != nullptr) {
                s_destructors[i].fn(ptr);
            }
            break;
        }
    }

    // 2. Built-in type cases
    switch (typeId) {
        case TYPE_PROBABLE:
        case TYPE_PROBABLE_ARRAY:
            break;

        default:
            break;
    }

    // Once the type-specific destructor finishes, we reclaim the raw block.
    Memory_free(ptr);
}
