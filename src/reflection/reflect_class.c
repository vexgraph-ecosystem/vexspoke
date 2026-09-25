// reflection/reflect_class.c — the Class reflection record.

#include "reflection/reflect_class.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReflectClass
 * ============================================================================
 * The Class kind of reflection: a 40-byte record holding a folded name, a
 * constructor (void* (*)(void*)), and a schema pointer (the class descriptor).
 * The kind is the header typeId (TYPE_REFLECT_CLASS). Records are stored
 * segregated by kind (one homogeneous array per kind) per the BitPool Slot
 * Segregation Law.
 *
 * Lifetime: arena-allocated (TYPE_REFLECT_CLASS) or embedded. Cold path only
 * (the Cold-Only Reflection Law); every getter is null-safe.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReflectClass (reflection/reflect_class.c)
 * LEVEL: L2 — Behavior (reflection metadata)
 * ============================================================================
 * the Class reflection record (name + constructor + schema).
 *
 * STRUCT FIELDS (Mirroring reflection/reflect_class.h):
 * ----------------------------------------------------------------------------
 *   ReflectClass {
 *     char name[24];                    // folded name (atom grammar)
 *     ReflectClassConstructFn construct; // the constructor
 *     void *schema;                     // the class descriptor
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure logic, no behavior of their own): none.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - ReflectClass_0() .. _3(name, construct, schema)
 *   - ReflectClass_init(self, name, construct, schema)
 *   - ReflectClass_free(self)
 *
 * Public Core Functions: (.h)
 *   - ReflectClass_kind(self)
 *   - ReflectClass_check(self, typeId)
 *   - ReflectClass_construct(self, arg)
 *
 * Public Setters: (.h)
 *   - ReflectClass_setName / _setConstruct / _setSchema
 *
 * Public Getters: (.h)
 *   - ReflectClass_getName / _getConstruct / _getSchema
 *
 * Public String Projections: (.h)
 *   - ReflectClass_toString / _toStringStruct
 * ============================================================================
 */

// CONSTRUCTORS

bool ReflectClass_init(ReflectClass *self, const char *name, ReflectClassConstructFn construct, void *schema) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_CLASS_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset(self, 0, sizeof(*self));
    memcpy((*self).name, folded, strlen(folded) + 1);
    (*self).construct = construct;
    (*self).schema = schema;
    return true;
}

static ReflectClass *instant(void) {
    ReflectClass *self = (ReflectClass*) Memory_alloc(TYPE_REFLECT_CLASS, sizeof(ReflectClass));
    if (self == nullptr)
        return nullptr;
    memset(self, 0, sizeof(*self));
    return self;
}

ReflectClass *ReflectClass_0(void) {
    return instant();
}

ReflectClass *ReflectClass_1(const char *name) {
    return ReflectClass_3(name, nullptr, nullptr);
}

ReflectClass *ReflectClass_2(const char *name, ReflectClassConstructFn construct) {
    return ReflectClass_3(name, construct, nullptr);
}

ReflectClass *ReflectClass_3(const char *name, ReflectClassConstructFn construct, void *schema) {
    ReflectClass *self = instant();
    if (self == nullptr)
        return nullptr;
    if (!ReflectClass_init(self, name, construct, schema)) {
        Memory_free(self);
        return nullptr;
    }
    return self;
}

void ReflectClass_free(ReflectClass *self) {
    if (self != nullptr)
        Memory_free(self);
}

// CORE FUNCTIONS

uint64_t ReflectClass_kind(const ReflectClass *self) {
    if (self == nullptr)
        return 0u;
    return Memory_type((void*) self);
}

bool ReflectClass_check(const ReflectClass *self, uint64_t typeId) {
    if (self == nullptr)
        return false;
    return Memory_type((void*) self) == typeId;
}

void *ReflectClass_construct(ReflectClass *self, void *arg) {
    if (self == nullptr || (*self).construct == nullptr)
        return nullptr;
    return (*self).construct(arg);
}

// SETTERS

bool ReflectClass_setName(ReflectClass *self, const char *name) {
    if (self == nullptr)
        return false;
    char folded[REFLECT_CLASS_NAME_BYTES];
    if (!VariableSlot_foldName(name, folded))
        return false;
    memset((*self).name, 0, sizeof((*self).name));
    memcpy((*self).name, folded, strlen(folded) + 1);
    return true;
}

void ReflectClass_setConstruct(ReflectClass *self, ReflectClassConstructFn construct) {
    if (self == nullptr)
        return;
    (*self).construct = construct;
}

void ReflectClass_setSchema(ReflectClass *self, void *schema) {
    if (self == nullptr)
        return;
    (*self).schema = schema;
}

// GETTERS

int ReflectClass_getName(const ReflectClass *self, char *out, size_t outCap) {
    if (self == nullptr || out == nullptr || outCap == 0u)
        return -1;
    const char *src = (*self).name;
    size_t len = strlen(src);
    if (len + 1u > outCap)
        return -1;
    memcpy(out, src, len + 1u);
    return (int) len;
}

ReflectClassConstructFn ReflectClass_getConstruct(const ReflectClass *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).construct;
}

void *ReflectClass_getSchema(const ReflectClass *self) {
    if (self == nullptr)
        return nullptr;
    return (*self).schema;
}

// STRING PROJECTIONS (the toString Law)

void ReflectClass_toString(const ReflectClass *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectClass(%s)",
                           (*self).name[0] == '\0' ? "?" : (*self).name);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}

void ReflectClass_toStringStruct(const ReflectClass *self, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated)
        *outTruncated = false;
    if (dest == nullptr || cap == 0u)
        return;
    if (self == nullptr) {
        snprintf(dest, cap, "nullptr");
        return;
    }
    int written = snprintf(dest, cap, "ReflectClass { name=\"%s\", construct=0x%llx, schema=0x%llx }",
                           (*self).name,
                           (unsigned long long) (uintptr_t) (*self).construct,
                           (unsigned long long) (uintptr_t) (*self).schema);
    if (written < 0 || (size_t) written >= cap) {
        if (outTruncated)
            *outTruncated = true;
    }
}
