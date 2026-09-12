// relational/variable.c — Variable registry port (Legacy: variable/Variable.java).
//
// Rows reference shared pool slots by index (never inline names); lookup
// goes pool-first, then one sparse hop (slot -> var id). Typed queries
// filter rows by classId — one structure, no segregated lists.

#include "relational/variable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "annotation/overview.h"
#include "nio/mem.h"
#include "relational/variable_pool.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Variable (relational/variable.c)
 * LEVEL: L2 — Behavior (relational behavior API)
 * ============================================================================
 * the relational symbol registry (Legacy: variable/Variable.java).
 *
 * STRUCT FIELDS (Mirroring relational/variable.h):
 * ----------------------------------------------------------------------------
 *   Variable {
 *     bool active; // runtime-active flag
 *     VariableRow *rows; // dense rows, varId == index (append-only)
 *     uint32_t count; // live rows
 *     uint32_t capacity; // allocated rows
 *     int32_t *bySlot; // pool slot -> varId, -1 empty (sparse, grown)
 *     uint32_t bySlotCap; // bySlot slots allocated
 *   }
 *
 * SLOT RECORD (owned by Variable, behaviorless):
 * ----------------------------------------------------------------------------
 *   VariableRow slot; // int32_t + pool slot index (name lives in pool)
 *   VariableRow classId; // uint32_t + value class, pinned at creation
 *   VariableRow pointer; // uintptr_t + the value
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Variable_init(v)
 *   - Variable_shutdown(v)
 *
 * Core Functions:
 *   - Variable_instant(v, name, classId, targetPointer)
 *   - Variable_rename(v, oldName, newName)
 *   - Variable_findByClass(v, classId, outIds, cap)
 *   - Variable_compareAndSetPointer(v, varId, expected, newPointer)
 *
 * Setters:
 *   - Variable_setPointer(v, varId, targetPointer)
 *
 * Getters:
 *   - Variable_getId(v, name)
 *   - Variable_getPointer(v, varId)
 *   - Variable_getClassId(v, varId)
 *   - Variable_getName(v, varId, out, outCap)
 *   - Variable_getActiveCount(v)
 * ============================================================================
 */

// Validate + fold to lowercase for pool space. Dots ride along for dotted
// paths (character.position.x); everything outside ASCII alnum/underscore/
// dot — including multibyte UTF-8/16 garbage — is rejected cold.
static bool clean_name(const char *name, char *lowered) {
    if (!name || name[0] == '\0')
        return false;
    size_t len = strlen(name);
    if (len > STRING_POOL_NAME_MAX)
        return false;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) name[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '.';
        if (!ok)
            return false;
        lowered[i] = (c >= 'A' && c <= 'Z') ? (char) (c + 32) : (char) c;
    }
    lowered[len] = '\0';
    return true;
}

static VariableRow *row_at(Variable *v, int32_t varId) {
    if (!v || !(*v).active || !(*v).rows || varId < 0 || (uint32_t) varId >= (*v).count)
        return nullptr;
    VariableRow *rows = (*v).rows;
    return &rows[varId];
}

// Pool slot -> var id through the sparse hop. -1 when unmapped (a pool slot
// may exist globally without belonging to this scope).
static int32_t slot_var(Variable *v, int32_t slot) {
    if (slot < 0 || (uint32_t) slot >= (*v).bySlotCap)
        return -1;
    int32_t *bySlot = (*v).bySlot;
    return bySlot[slot];
}

static bool ensure_byslot(Variable *v, uint32_t need) {
    if (need < (*v).bySlotCap)
        return true;
    uint32_t cap = (*v).bySlotCap ? (*v).bySlotCap * 2 : 64;
    while (cap <= need) {
        if (cap > UINT32_MAX / 2)
            return false;
        cap *= 2;
    }
    int32_t *next = (int32_t*) malloc((size_t) cap * sizeof(int32_t));
    if (!next)
        return false;
    for (uint32_t i = 0; i < cap; i++)
        next[i] = -1;
    if ((*v).bySlot) {
        memcpy(next, (*v).bySlot, (size_t)(*v).bySlotCap * sizeof(int32_t));
        free((*v).bySlot);
    }
    (*v).bySlot = next;
    (*v).bySlotCap = cap;
    return true;
}

bool Variable_init(Variable *v) {
    if (!v)
        return false;
    memset(v, 0, sizeof(*v));
    MemoryArena *arena = Memory_defaultArena();
    if (!arena)
        return false;
    if (!StringPool_init(arena))
        return false;
    (*v).rows = (VariableRow*) malloc(VARIABLE_DEFAULT_CAPACITY * sizeof(VariableRow));
    if (!(*v).rows)
        return false;
    (*v).capacity = VARIABLE_DEFAULT_CAPACITY;
    (*v).active = true;
    return true;
}

void Variable_shutdown(Variable *v) {
    if (!v || !(*v).active)
        return;
    free((*v).rows);
    free((*v).bySlot);
    (*v).rows = nullptr;
    (*v).bySlot = nullptr;
    (*v).count = 0;
    (*v).capacity = 0;
    (*v).bySlotCap = 0;
    (*v).active = false;
}

int32_t Variable_instant(Variable *v, const char *name, uint32_t classId, uintptr_t targetPointer) {
    if (!v || !(*v).active) {
        fprintf(stderr, "[variable] instant: inactive registry\n");
        return -1;
    }
    if (!name || name[0] == '\0') {
        fprintf(stderr, "[variable] instant: empty name rejected\n");
        return -1;
    }
    char lowered[24];
    size_t rawLen = strlen(name);
    if (rawLen > STRING_POOL_NAME_MAX) {
        fprintf(stderr, "[variable] instant: name too long ('%.32s', max 23)\n", name);
        return -1;
    }
    if (!clean_name(name, lowered)) {
        fprintf(stderr, "[variable] instant: illegal name ('%.32s'): ascii alnum, underscore, dot only\n", name);
        return -1;
    }
    int32_t slot = StringPool_find(lowered);
    if (slot >= 0 && slot_var(v, slot) >= 0) {
        fprintf(stderr, "[variable] instant: name already registered ('%s')\n", lowered);
        return -1;
    }
    if (!ensure_byslot(v, StringPool_count())) {
        fprintf(stderr, "[variable] instant: out of memory ('%s')\n", lowered);
        return -1;
    }
    slot = StringPool_intern(lowered);
    if (slot < 0) {
        fprintf(stderr, "[variable] instant: intern failed ('%s')\n", lowered);
        return -1;
    }
    if ((*v).count >= (*v).capacity) {
        size_t newCap = (size_t)(*v).capacity + VARIABLE_DEFAULT_CAPACITY;
        VariableRow *next = (VariableRow*) realloc((*v).rows, newCap * sizeof(VariableRow));
        if (!next) {
            fprintf(stderr, "[variable] instant: out of memory ('%s')\n", lowered);
            return -1;
        }
        (*v).rows = next;
        (*v).capacity = (uint32_t) newCap;
    }
    uint32_t id = (*v).count;
    VariableRow *rows = (*v).rows;
    rows[id].slot = slot;
    rows[id].classId = classId;
    rows[id].pointer = targetPointer;
    int32_t *bySlot = (*v).bySlot;
    bySlot[slot] = (int32_t) id;
    (*v).count++;
    return (int32_t) id;
}

int32_t Variable_getId(Variable *v, const char *name) {
    if (!v || !(*v).active || !name)
        return -1;
    char lowered[24];
    if (!clean_name(name, lowered))
        return -1;
    int32_t slot = StringPool_find(lowered);
    if (slot < 0)
        return -1;
    return slot_var(v, slot);
}

bool Variable_rename(Variable *v, const char *oldName, const char *newName) {
    if (!v || !(*v).active) {
        fprintf(stderr, "[variable] rename: inactive registry\n");
        return false;
    }
    char oldLower[24];
    char newLower[24];
    if (!oldName || !clean_name(oldName, oldLower)) {
        fprintf(stderr, "[variable] rename: bad old name\n");
        return false;
    }
    if (!newName || !clean_name(newName, newLower)) {
        fprintf(stderr, "[variable] rename: bad new name\n");
        return false;
    }
    int32_t oldSlot = StringPool_find(oldLower);
    int32_t id = oldSlot >= 0 ? slot_var(v, oldSlot) : -1;
    if (id < 0) {
        fprintf(stderr, "[variable] rename: unknown name ('%s')\n", oldLower);
        return false;
    }
    int32_t newSlot = StringPool_find(newLower);
    if (newSlot >= 0 && slot_var(v, newSlot) >= 0) {
        fprintf(stderr, "[variable] rename: name already registered ('%s')\n", newLower);
        return false;
    }
    if (!ensure_byslot(v, StringPool_count())) {
        fprintf(stderr, "[variable] rename: out of memory ('%s')\n", newLower);
        return false;
    }
    newSlot = StringPool_intern(newLower);
    if (newSlot < 0) {
        fprintf(stderr, "[variable] rename: intern failed ('%s')\n", newLower);
        return false;
    }
    VariableRow *rows = (*v).rows;
    int32_t *bySlot = (*v).bySlot;
    int32_t oldSlotIdx = rows[id].slot;
    rows[id].slot = newSlot;
    if (oldSlotIdx >= 0 && (uint32_t) oldSlotIdx < (*v).bySlotCap && bySlot[oldSlotIdx] == id)
        bySlot[oldSlotIdx] = -1;
    bySlot[newSlot] = id;
    return true;
}

size_t Variable_findByClass(Variable *v, uint32_t classId, int32_t *outIds, size_t cap) {
    if (!v || !(*v).active)
        return 0;
    size_t total = 0;
    VariableRow *rows = (*v).rows;
    for (uint32_t i = 0; i < (*v).count; i++) {
        if (rows[i].classId == classId) {
            if (outIds && total < cap)
                outIds[total] = (int32_t) i;
            total++;
        }
    }
    return total;
}

uintptr_t Variable_getPointer(Variable *v, int32_t varId) {
    VariableRow *row = row_at(v, varId);
    return row ? (*row).pointer : 0;
}

void Variable_setPointer(Variable *v, int32_t varId, uintptr_t targetPointer) {
    VariableRow *row = row_at(v, varId);
    if (!row)
        return;
    (*row).pointer = targetPointer;
}

bool Variable_compareAndSetPointer(Variable *v, int32_t varId, uintptr_t expected, uintptr_t newPointer) {
    VariableRow *row = row_at(v, varId);
    if (!row)
        return false;
    if ((*row).pointer != expected)
        return false;
    (*row).pointer = newPointer;
    return true;
}

uint32_t Variable_getClassId(Variable *v, int32_t varId) {
    VariableRow *row = row_at(v, varId);
    return row ? (*row).classId : 0;
}

int Variable_getName(Variable *v, int32_t varId, char *out, size_t outCap) {
    VariableRow *row = row_at(v, varId);
    if (!row || !out)
        return -1;
    const char *name = StringPool_name((uint32_t) (*row).slot);
    if (!name)
        return -1;
    size_t len = strlen(name);
    if (outCap < len + 1)
        return -1;
    memcpy(out, name, len + 1);
    return (int)len;
}

size_t Variable_getActiveCount(Variable *v) {
    if (!v || !(*v).active)
        return 0;
    return (*v).count;
}
