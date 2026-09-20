#include "objects/local.h"

#include <string.h>

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Local
 * ============================================================================
 * Thread-local variable slot table object wrapper. Values live in an
 * arena-backed table indexed by thread id; the table starts at 8 slots and
 * doubles on demand (the Dynamic Scalability & Anti-Hardcoding Law), so the
 * thread-id space is never capped. Reads outside the live table return 0;
 * writes grow the table first. Local_2 deep-copies each element's table so
 * array instances own independent slot storage.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Local (objects/local.c)
 * LEVEL: L2 — Behavior (object behavior API)
 * ============================================================================
 * Thread-local variable slot table object wrapper. Values live in an
 * arena-backed table indexed by thread id; the table starts at 8 slots and
 * doubles on demand (the Dynamic Scalability & Anti-Hardcoding Law), so the
 * thread-id space is never capped. Reads outside the live table return 0;
 * writes grow the table first.
 *
 * STRUCT FIELDS (local to this file):
 * ----------------------------------------------------------------------------
 *   Local {
 *     uint64_t *slots;   // arena-backed value table, indexed by threadId
 *     size_t slotCap;    // live table size (grows by doubling)
 *   }
 *
 * PRIVATE HELPERS (kept file-local pure-data only, each with full fields):
 * ----------------------------------------------------------------------------
 *   local_grow(self, threadId)   // double slotCap until threadId fits,
 *                                // zero new tail, free old table
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Local_0(void)
 *   - Local_2(init, count)
 *
 * Core Functions:
 *   - Local_free(local)
 *
 * Setters:
 *   - Local_set(local, threadId, value)
 *
 * Getters:
 *   - Local_get(local, threadId)
 * ============================================================================
 */


// local.c — Thread-local variable slot table object wrapper implementation.

typedef struct Local {
    uint64_t *slots;   // arena-backed value table, indexed by threadId
    size_t slotCap;    // live table size (grows by doubling)
} Local;

// Grow the slot table until threadId fits; the new tail is zeroed. On OOM
// the table is untouched and false is returned (the Cold-Strict,
// Hot-Minimal Validation Law drop-degrade).
static bool local_grow(Local *self, uint32_t threadId) {
    size_t need = (size_t) threadId + 1;
    size_t newCap = (*self).slotCap ? (*self).slotCap : 8;
    while (newCap < need) {
        if (newCap > SIZE_MAX / 2)
            return false;
        newCap *= 2;
    }
    uint64_t type = Type_make(PROJ_VEXSPOKE, FORM_SINGLETON, ID_LOCAL) | MOD_LOCALE;
    uint64_t *nb = (uint64_t*) Memory_alloc(type, newCap * sizeof(uint64_t));
    if (!nb)
        return false;
    if ((*self).slots) {
        memcpy(nb, (*self).slots, (*self).slotCap * sizeof(uint64_t));
        Memory_free((*self).slots);
    }
    memset(nb + (*self).slotCap, 0, (newCap - (*self).slotCap) * sizeof(uint64_t));
    (*self).slots = nb;
    (*self).slotCap = newCap;
    return true;
}

Local *Local_0(void) {
    uint64_t type = Type_make(PROJ_VEXSPOKE, FORM_SINGLETON, ID_LOCAL) | MOD_LOCALE;
    Local *local = (Local*) Memory_alloc(type, sizeof(Local));
    if (!local)
        return nullptr;
    (*local).slots = nullptr;
    (*local).slotCap = 0;
    return local;
}


Local *Local_2(const Local *init, size_t count) {
    if (count == 0) return nullptr;
    Local *p = (Local*) Memory_alloc(TYPE_LOCAL_ARRAY, sizeof(Local) * count);
    if (!p) return nullptr;
    for (size_t i = 0; i < count; i++) {
        if (init) {
            // Deep copy: each element owns a table of init's live size.
            p[i].slots = nullptr;
            p[i].slotCap = 0;
            if ((*init).slotCap) {
                if (!local_grow(&p[i], (uint32_t) ((*init).slotCap - 1))) {
                    for (size_t f = 0; f < i; f++) Memory_free(p[f].slots);
                    Memory_free(p);
                    return nullptr;
                }
                memcpy(p[i].slots, (*init).slots, (*init).slotCap * sizeof(uint64_t));
            }
        } else {
            p[i].slots = nullptr;
            p[i].slotCap = 0;
        }
    }
    return p;
}
void Local_free(Local *local) {
    if (!local) return;
    if ((*local).slots)
        Memory_free((*local).slots);
    Memory_free(local);
}

uint64_t Local_get(const Local *local, uint32_t threadId) {
    if (!local || !(*local).slots || threadId >= (*local).slotCap)
        return 0;
    return (*local).slots[threadId];
}

void Local_set(Local *local, uint32_t threadId, uint64_t value) {
    if (!local)
        return;
    if (threadId >= (*local).slotCap) {
        if (!local_grow(local, threadId))
            return;
    }
    (*local).slots[threadId] = value;
}
