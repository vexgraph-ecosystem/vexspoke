// reflection/reflect_method.c — the Method reflection record.
//
// Fold + validate the name through the atom, keep the callable and target
// opaque. No allocation on the read path.

#include "reflection/reflect_method.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReflectMethod
 * ============================================================================
 * The Method kind of reflection: a 40-byte record holding a folded name (the
 * atom's 24-byte grammar), a void*(*)(void*) callable, and a target. The kind
 * is the header typeId (TYPE_REFLECT_METHOD), so identity is free. Records are
 * stored segregated by kind (one homogeneous array per kind) per the BitPool
 * Slot Segregation Law; this file owns only the Method shape.
 *
 * Lifetime: arena-allocated (TYPE_REFLECT_METHOD) or embedded. Cold path only
 * (the Cold-Only Reflection Law); every getter is null-safe.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReflectMethod (reflection/reflect_method.c)
 * LEVEL: L2 — Behavior (reflection metadata)
 * ============================================================================
 * the Method reflection record (name + callable + target).
 *
 * STRUCT FIELDS (Mirroring reflection/reflect_method.h):
 * ----------------------------------------------------------------------------
 *   ReflectMethod {
 *     char name[24];       // folded name (atom grammar)
 *     ReflectMethodFn invoke; // the callable
 *     void *target;        // the owner / default receiver
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure logic, no behavior of their own): none.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - ReflectMethod_0() .. _3(name, invoke, target)
 *   - ReflectMethod_init(self, name, invoke, target)
 *   - ReflectMethod_free(self)
 *
 * Public Core Functions: (.h)
 *   - ReflectMethod_kind(self)
 *   - ReflectMethod_check(self, typeId)
 *   - ReflectMethod_call(self, arg)
 *
 * Public Setters: (.h)
 *   - ReflectMethod_setName(self, name)
 *   - ReflectMethod_setInvoke(self, invoke)
 *   - ReflectMethod_setTarget(self, target)
 *
 * Public Getters: (.h)
 *   - ReflectMethod_getName(self, out, outCap)
 *   - ReflectMethod_getInvoke(self)
 *   - ReflectMethod_getTarget(self)
 *
 * Public String Projections: (.h)
 *   - ReflectMethod_toString(self, dest, cap, outTruncated)
 *   - ReflectMethod_toStringStruct(self, dest, cap, outTruncated)
 * ============================================================================
 */

// CONSTRUCTORS

bool ReflectMethod_init(ReflectMethod *self, const char *name, ReflectMethodFn invoke, void *target) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_METHOD_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset(self, 0, sizeof(*self));
    memcpy((*self).name, folded, strlen(folded) + 1);
    (*self).invoke = invoke;
    (*self).target = target;
    return true;
}

static ReflectMethod *instant(void) {
    ReflectMethod *self = (ReflectMethod*) Memory_alloc(TYPE_REFLECT_METHOD, sizeof(ReflectMethod));
    if (self == nullptr)
        return nullptr;
    memset(self, 0, sizeof(*self));
    return self;
}

ReflectMethod *ReflectMethod_0(void) {
    return instant();
}

ReflectMethod *ReflectMethod_1(const char *name) {
    return ReflectMethod_3(name, nullptr, nullptr);
}

ReflectMethod *ReflectMethod_2(const char *name, ReflectMethodFn invoke) {
    return ReflectMethod_3(name, invoke, nullptr);
}

ReflectMethod *ReflectMethod_3(const char *name, ReflectMethodFn invoke, void *target) {
    ReflectMethod *self = instant();
    if (self == nullptr)
        return nullptr;
    if (!ReflectMethod_init(self, name, invoke, target)) {
        Memory_free(self);
        return nullptr;
    }
    return self;
}

void ReflectMethod_free(ReflectMethod *self) {
    if (self != nullptr)
        Memory_free(self);
}

// CORE FUNCTIONS

uint64_t ReflectMethod_kind(const ReflectMethod *self) {
    if (self == nullptr)
        return 0u;
    return Memory_type((void*) self);
}

bool ReflectMethod_check(const ReflectMethod *self, uint64_t typeId) {
    if (self == nullptr)
        return false;
    return Memory_type((void*) self) == typeId;
}

void *ReflectMethod_call(ReflectMethod *self, void *arg) {
    if (self == nullptr || (*self).invoke == nullptr)
        return nullptr;
    return (*self).invoke(arg);
}

// SETTERS

bool ReflectMethod_setName(ReflectMethod *self, const char *name) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_METHOD_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset((*self).name, 0, sizeof((*self).name));
    memcpy((*self).name, folded, strlen(folded) + 1);
    return true;
}

void ReflectMethod_setInvoke(ReflectMethod *self, ReflectMethodFn invoke) {
    if (self == nullptr)
        return;
    (*self).invoke = invoke;
}

void ReflectMethod_setTarget(ReflectMethod *self, void *target) {
    if (self == nullptr)
        return;
    (*self).target = target;
}

// GETTERS

int ReflectMethod_getName(const ReflectMethod *self, char *out, size_t outCap) {
    if (self == nullptr || out == nullptr || outCap == 0u)
        return -1;
    const char *src = (*self).name;
    size_t len = strlen(src);
    if (len + 1u > outCap)
        return -1;
    memcpy(out, src, len + 1u);
    return (int) len;
}

ReflectMethodFn ReflectMethod_getInvoke(const ReflectMethod *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).invoke;
}

void *ReflectMethod_getTarget(const ReflectMethod *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).target;
}

// STRING PROJECTIONS (the toString Law)

void ReflectMethod_toString(const ReflectMethod *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectMethod(%s -> 0x%llx)",
                           (*self).name[0] == '\0' ? "?" : (*self).name,
                           (unsigned long long) (uintptr_t) (*self).invoke);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void ReflectMethod_toStringStruct(const ReflectMethod *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectMethod { name=\"%s\", invoke=0x%llx, target=0x%llx }",
                           (*self).name,
                           (unsigned long long) (uintptr_t) (*self).invoke,
                           (unsigned long long) (uintptr_t) (*self).target);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
