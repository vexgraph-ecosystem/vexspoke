// reflection/reflect_variable.c — the Variable reflection record.

#include "reflection/reflect_variable.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReflectVariable
 * ============================================================================
 * The Variable kind of reflection: a 40-byte record holding a folded name, a
 * reader (void* (*)(void*)), and a target (the value / owner). The kind is the
 * header typeId (TYPE_REFLECT_VARIABLE). Records are stored segregated by kind
 * (one homogeneous array per kind) per the BitPool Slot Segregation Law.
 *
 * Lifetime: arena-allocated (TYPE_REFLECT_VARIABLE) or embedded. Cold path only
 * (the Cold-Only Reflection Law); every getter is null-safe.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReflectVariable (reflection/reflect_variable.c)
 * LEVEL: L2 — Behavior (reflection metadata)
 * ============================================================================
 * the Variable reflection record (name + reader + target).
 *
 * STRUCT FIELDS (Mirroring reflection/reflect_variable.h):
 * ----------------------------------------------------------------------------
 *   ReflectVariable {
 *     char name[24];              // folded name (atom grammar)
 *     ReflectVariableReadFn read; // the reader
 *     void *target;               // the value / owner
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure logic, no behavior of their own): none.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - ReflectVariable_0() .. _3(name, read, target)
 *   - ReflectVariable_init(self, name, read, target)
 *   - ReflectVariable_free(self)
 *
 * Public Core Functions: (.h)
 *   - ReflectVariable_kind(self)
 *   - ReflectVariable_check(self, typeId)
 *   - ReflectVariable_read(self, arg)
 *
 * Public Setters: (.h)
 *   - ReflectVariable_setName / _setRead / _setTarget
 *
 * Public Getters: (.h)
 *   - ReflectVariable_getName / _getRead / _getTarget
 *
 * Public String Projections: (.h)
 *   - ReflectVariable_toString / _toStringStruct
 * ============================================================================
 */

// CONSTRUCTORS

bool ReflectVariable_init(ReflectVariable *self, const char *name, ReflectVariableReadFn read, void *target) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_VARIABLE_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset(self, 0, sizeof(*self));
    memcpy((*self).name, folded, strlen(folded) + 1);
    (*self).read = read;
    (*self).target = target;
    return true;
}

static ReflectVariable *instant(void) {
    ReflectVariable *self = (ReflectVariable*) Memory_alloc(TYPE_REFLECT_VARIABLE, sizeof(ReflectVariable));
    if (self == nullptr)
        return nullptr;
    memset(self, 0, sizeof(*self));
    return self;
}

ReflectVariable *ReflectVariable_0(void) {
    return instant();
}

ReflectVariable *ReflectVariable_1(const char *name) {
    return ReflectVariable_3(name, nullptr, nullptr);
}

ReflectVariable *ReflectVariable_2(const char *name, ReflectVariableReadFn read) {
    return ReflectVariable_3(name, read, nullptr);
}

ReflectVariable *ReflectVariable_3(const char *name, ReflectVariableReadFn read, void *target) {
    ReflectVariable *self = instant();
    if (self == nullptr)
        return nullptr;
    if (!ReflectVariable_init(self, name, read, target)) {
        Memory_free(self);
        return nullptr;
    }
    return self;
}

void ReflectVariable_free(ReflectVariable *self) {
    if (self != nullptr)
        Memory_free(self);
}

// CORE FUNCTIONS

uint64_t ReflectVariable_kind(const ReflectVariable *self) {
    if (self == nullptr)
        return 0u;
    return Memory_type((void*) self);
}

bool ReflectVariable_check(const ReflectVariable *self, uint64_t typeId) {
    if (self == nullptr)
        return false;
    return Memory_type((void*) self) == typeId;
}

void *ReflectVariable_read(ReflectVariable *self, void *arg) {
    if (self == nullptr || (*self).read == nullptr)
        return nullptr;
    return (*self).read(arg);
}

// SETTERS

bool ReflectVariable_setName(ReflectVariable *self, const char *name) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_VARIABLE_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset((*self).name, 0, sizeof((*self).name));
    memcpy((*self).name, folded, strlen(folded) + 1);
    return true;
}

void ReflectVariable_setRead(ReflectVariable *self, ReflectVariableReadFn read) {
    if (self == nullptr)
        return;
    (*self).read = read;
}

void ReflectVariable_setTarget(ReflectVariable *self, void *target) {
    if (self == nullptr)
        return;
    (*self).target = target;
}

// GETTERS

int ReflectVariable_getName(const ReflectVariable *self, char *out, size_t outCap) {
    if (self == nullptr || out == nullptr || outCap == 0u)
        return -1;
    const char *src = (*self).name;
    size_t len = strlen(src);
    if (len + 1u > outCap)
        return -1;
    memcpy(out, src, len + 1u);
    return (int) len;
}

ReflectVariableReadFn ReflectVariable_getRead(const ReflectVariable *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).read;
}

void *ReflectVariable_getTarget(const ReflectVariable *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).target;
}

// STRING PROJECTIONS (the toString Law)

void ReflectVariable_toString(const ReflectVariable *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectVariable(%s)",
                           (*self).name[0] == '\0' ? "?" : (*self).name);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void ReflectVariable_toStringStruct(const ReflectVariable *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectVariable { name=\"%s\", read=0x%llx, target=0x%llx }",
                           (*self).name,
                           (unsigned long long) (uintptr_t) (*self).read,
                           (unsigned long long) (uintptr_t) (*self).target);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
