// reflection/reflect_generic.c — the Generic reflection record.

#include "reflection/reflect_generic.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReflectGeneric
 * ============================================================================
 * The Generic kind of reflection: a 40-byte record for an untyped callable — a
 * folded name, a void*(*)(void*) function, and a target. It is the escape hatch
 * for a behavior that is none of class/field/variable/method. The kind is the
 * header typeId (TYPE_REFLECT_GENERIC). Records are stored segregated by kind
 * (one homogeneous array per kind) per the BitPool Slot Segregation Law.
 *
 * Lifetime: arena-allocated (TYPE_REFLECT_GENERIC) or embedded. Cold path only
 * (the Cold-Only Reflection Law); every getter is null-safe.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReflectGeneric (reflection/reflect_generic.c)
 * LEVEL: L2 — Behavior (reflection metadata)
 * ============================================================================
 * the Generic reflection record (name + callable + target).
 *
 * STRUCT FIELDS (Mirroring reflection/reflect_generic.h):
 * ----------------------------------------------------------------------------
 *   ReflectGeneric {
 *     char name[24];       // folded name (atom grammar)
 *     ReflectGenericFn fn; // the callable
 *     void *target;        // the referent
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure logic, no behavior of their own): none.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - ReflectGeneric_0() .. _3(name, fn, target)
 *   - ReflectGeneric_init(self, name, fn, target)
 *   - ReflectGeneric_free(self)
 *
 * Public Core Functions: (.h)
 *   - ReflectGeneric_kind(self)
 *   - ReflectGeneric_check(self, typeId)
 *   - ReflectGeneric_call(self, arg)
 *
 * Public Setters: (.h)
 *   - ReflectGeneric_setName / _setFn / _setTarget
 *
 * Public Getters: (.h)
 *   - ReflectGeneric_getName / _getFn / _getTarget
 *
 * Public String Projections: (.h)
 *   - ReflectGeneric_toString / _toStringStruct
 * ============================================================================
 */

// CONSTRUCTORS

bool ReflectGeneric_init(ReflectGeneric *self, const char *name, ReflectGenericFn fn, void *target) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_GENERIC_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset(self, 0, sizeof(*self));
    memcpy((*self).name, folded, strlen(folded) + 1);
    (*self).fn = fn;
    (*self).target = target;
    return true;
}

static ReflectGeneric *instant(void) {
    ReflectGeneric *self = (ReflectGeneric*) Memory_alloc(TYPE_REFLECT_GENERIC, sizeof(ReflectGeneric));
    if (self == nullptr)
        return nullptr;
    memset(self, 0, sizeof(*self));
    return self;
}

ReflectGeneric *ReflectGeneric_0(void) {
    return instant();
}

ReflectGeneric *ReflectGeneric_1(const char *name) {
    return ReflectGeneric_3(name, nullptr, nullptr);
}

ReflectGeneric *ReflectGeneric_2(const char *name, ReflectGenericFn fn) {
    return ReflectGeneric_3(name, fn, nullptr);
}

ReflectGeneric *ReflectGeneric_3(const char *name, ReflectGenericFn fn, void *target) {
    ReflectGeneric *self = instant();
    if (self == nullptr)
        return nullptr;
    if (!ReflectGeneric_init(self, name, fn, target)) {
        Memory_free(self);
        return nullptr;
    }
    return self;
}

void ReflectGeneric_free(ReflectGeneric *self) {
    if (self != nullptr)
        Memory_free(self);
}

// CORE FUNCTIONS

uint64_t ReflectGeneric_kind(const ReflectGeneric *self) {
    if (self == nullptr)
        return 0u;
    return Memory_type((void*) self);
}

bool ReflectGeneric_check(const ReflectGeneric *self, uint64_t typeId) {
    if (self == nullptr)
        return false;
    return Memory_type((void*) self) == typeId;
}

void *ReflectGeneric_call(ReflectGeneric *self, void *arg) {
    if (self == nullptr || (*self).fn == nullptr)
        return nullptr;
    return (*self).fn(arg);
}

// SETTERS

bool ReflectGeneric_setName(ReflectGeneric *self, const char *name) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_GENERIC_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset((*self).name, 0, sizeof((*self).name));
    memcpy((*self).name, folded, strlen(folded) + 1);
    return true;
}

void ReflectGeneric_setFn(ReflectGeneric *self, ReflectGenericFn fn) {
    if (self == nullptr)
        return;
    (*self).fn = fn;
}

void ReflectGeneric_setTarget(ReflectGeneric *self, void *target) {
    if (self == nullptr)
        return;
    (*self).target = target;
}

// GETTERS

int ReflectGeneric_getName(const ReflectGeneric *self, char *out, size_t outCap) {
    if (self == nullptr || out == nullptr || outCap == 0u)
        return -1;
    const char *src = (*self).name;
    size_t len = strlen(src);
    if (len + 1u > outCap)
        return -1;
    memcpy(out, src, len + 1u);
    return (int) len;
}

ReflectGenericFn ReflectGeneric_getFn(const ReflectGeneric *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).fn;
}

void *ReflectGeneric_getTarget(const ReflectGeneric *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).target;
}

// STRING PROJECTIONS (the toString Law)

void ReflectGeneric_toString(const ReflectGeneric *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectGeneric(%s)",
                           (*self).name[0] == '\0' ? "?" : (*self).name);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void ReflectGeneric_toStringStruct(const ReflectGeneric *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectGeneric { name=\"%s\", fn=0x%llx, target=0x%llx }",
                           (*self).name,
                           (unsigned long long) (uintptr_t) (*self).fn,
                           (unsigned long long) (uintptr_t) (*self).target);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
