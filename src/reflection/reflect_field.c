// reflection/reflect_field.c — the Field reflection record.
//
// Fold + validate the name through the atom; keep the getter, setter and target
// opaque. No allocation on the read path.

#include "reflection/reflect_field.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReflectField
 * ============================================================================
 * The Field kind of reflection: a 48-byte record holding a folded name, a getter
 * and a setter (the only kind with two behaviors), and a target. The kind is the
 * header typeId (TYPE_REFLECT_FIELD). Records are stored segregated by kind (one
 * homogeneous array per kind) per the BitPool Slot Segregation Law, so this
 * 48-byte shape never shares a stride with the 40-byte kinds.
 *
 * Lifetime: arena-allocated (TYPE_REFLECT_FIELD) or embedded. Cold path only
 * (the Cold-Only Reflection Law); every getter is null-safe.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReflectField (reflection/reflect_field.c)
 * LEVEL: L2 — Behavior (reflection metadata)
 * ============================================================================
 * the Field reflection record (name + getter + setter + target).
 *
 * STRUCT FIELDS (Mirroring reflection/reflect_field.h):
 * ----------------------------------------------------------------------------
 *   ReflectField {
 *     char name[24];        // folded name (atom grammar)
 *     ReflectFieldGetFn get; // getter
 *     ReflectFieldSetFn set; // setter
 *     void *target;         // the field's owner / address
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure logic, no behavior of their own): none.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - ReflectField_0() .. _4(name, get, set, target)
 *   - ReflectField_init(self, name, get, set, target)
 *   - ReflectField_free(self)
 *
 * Public Core Functions: (.h)
 *   - ReflectField_kind(self)
 *   - ReflectField_check(self, typeId)
 *   - ReflectField_read(self, receiver)
 *   - ReflectField_write(self, receiver, value)
 *
 * Public Setters: (.h)
 *   - ReflectField_setName / _setGet / _setSet / _setTarget
 *
 * Public Getters: (.h)
 *   - ReflectField_getName / _getGet / _getSet / _getTarget
 *
 * Public String Projections: (.h)
 *   - ReflectField_toString / _toStringStruct
 * ============================================================================
 */

// CONSTRUCTORS

bool ReflectField_init(ReflectField *self, const char *name, ReflectFieldGetFn get, ReflectFieldSetFn set, void *target) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_FIELD_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset(self, 0, sizeof(*self));
    memcpy((*self).name, folded, strlen(folded) + 1);
    (*self).get = get;
    (*self).set = set;
    (*self).target = target;
    return true;
}

static ReflectField *instant(void) {
    ReflectField *self = (ReflectField*) Memory_alloc(TYPE_REFLECT_FIELD, sizeof(ReflectField));
    if (self == nullptr)
        return nullptr;
    memset(self, 0, sizeof(*self));
    return self;
}

ReflectField *ReflectField_0(void) {
    return instant();
}

ReflectField *ReflectField_1(const char *name) {
    return ReflectField_4(name, nullptr, nullptr, nullptr);
}

ReflectField *ReflectField_2(const char *name, ReflectFieldGetFn get) {
    return ReflectField_4(name, get, nullptr, nullptr);
}

ReflectField *ReflectField_3(const char *name, ReflectFieldGetFn get, ReflectFieldSetFn set) {
    return ReflectField_4(name, get, set, nullptr);
}

ReflectField *ReflectField_4(const char *name, ReflectFieldGetFn get, ReflectFieldSetFn set, void *target) {
    ReflectField *self = instant();
    if (self == nullptr)
        return nullptr;
    if (!ReflectField_init(self, name, get, set, target)) {
        Memory_free(self);
        return nullptr;
    }
    return self;
}

void ReflectField_free(ReflectField *self) {
    if (self != nullptr)
        Memory_free(self);
}

// CORE FUNCTIONS

uint64_t ReflectField_kind(const ReflectField *self) {
    if (self == nullptr)
        return 0u;
    return Memory_type((void*) self);
}

bool ReflectField_check(const ReflectField *self, uint64_t typeId) {
    if (self == nullptr)
        return false;
    return Memory_type((void*) self) == typeId;
}

void *ReflectField_read(ReflectField *self, void *receiver) {
    if (self == nullptr || (*self).get == nullptr)
        return nullptr;
    return (*self).get(receiver);
}

void ReflectField_write(ReflectField *self, void *receiver, void *value) {
    if (self == nullptr || (*self).set == nullptr)
        return;
    (*self).set(receiver, value);
}

// SETTERS

bool ReflectField_setName(ReflectField *self, const char *name) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_FIELD_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset((*self).name, 0, sizeof((*self).name));
    memcpy((*self).name, folded, strlen(folded) + 1);
    return true;
}

void ReflectField_setGet(ReflectField *self, ReflectFieldGetFn get) {
    if (self == nullptr)
        return;
    (*self).get = get;
}

void ReflectField_setSet(ReflectField *self, ReflectFieldSetFn set) {
    if (self == nullptr)
        return;
    (*self).set = set;
}

void ReflectField_setTarget(ReflectField *self, void *target) {
    if (self == nullptr)
        return;
    (*self).target = target;
}

// GETTERS

int ReflectField_getName(const ReflectField *self, char *out, size_t outCap) {
    if (self == nullptr || out == nullptr || outCap == 0u)
        return -1;
    const char *src = (*self).name;
    size_t len = strlen(src);
    if (len + 1u > outCap)
        return -1;
    memcpy(out, src, len + 1u);
    return (int) len;
}

ReflectFieldGetFn ReflectField_getGet(const ReflectField *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).get;
}

ReflectFieldSetFn ReflectField_getSet(const ReflectField *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).set;
}

void *ReflectField_getTarget(const ReflectField *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).target;
}

// STRING PROJECTIONS (the toString Law)

void ReflectField_toString(const ReflectField *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectField(%s)",
                           (*self).name[0] == '\0' ? "?" : (*self).name);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void ReflectField_toStringStruct(const ReflectField *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectField { name=\"%s\", get=0x%llx, set=0x%llx, target=0x%llx }",
                           (*self).name,
                           (unsigned long long) (uintptr_t) (*self).get,
                           (unsigned long long) (uintptr_t) (*self).set,
                           (unsigned long long) (uintptr_t) (*self).target);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
