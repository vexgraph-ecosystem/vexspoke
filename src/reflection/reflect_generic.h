#ifndef REFLECTION_REFLECT_GENERIC_H
#define REFLECTION_REFLECT_GENERIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "relational/variable_slot.h"

// reflection/reflect_generic.h — the Generic reflection record.
//
// The Generic kind of reflection: metadata for an untyped callable — a folded
// name (the atom's grammar), a function (void* (*)(void*)), and a target. It is
// the escape hatch for a behavior that is none of class/field/variable/method.
// The kind is the header typeId (TYPE_REFLECT_GENERIC). Block layout:
// [MemoryHeader 16][name 24][fn 8][target 8] = 56 bytes.

#define REFLECT_GENERIC_NAME_BYTES VARIABLE_SLOT_NAME_BYTES
#define REFLECT_GENERIC_NAME_MAX VARIABLE_SLOT_NAME_MAX
#define REFLECT_GENERIC_PAYLOAD_BYTES 40u

typedef void *(*ReflectGenericFn)(void *);

typedef struct ReflectGeneric {
    char name[REFLECT_GENERIC_NAME_BYTES]; // folded name (atom grammar)
    ReflectGenericFn fn;                   // the callable
    void *target;                          // the referent
} ReflectGeneric;

_Static_assert(sizeof(ReflectGeneric) == REFLECT_GENERIC_PAYLOAD_BYTES, "ReflectGeneric must stay 40 bytes");

bool ReflectGeneric_init(ReflectGeneric *self, const char *name, ReflectGenericFn fn, void *target);

ReflectGeneric *ReflectGeneric_0(void);
ReflectGeneric *ReflectGeneric_1(const char *name);
ReflectGeneric *ReflectGeneric_2(const char *name, ReflectGenericFn fn);
ReflectGeneric *ReflectGeneric_3(const char *name, ReflectGenericFn fn, void *target);
#define ReflectGeneric(...) CONSTRUCTOR_DISPATCH(ReflectGeneric, __VA_ARGS__)
void ReflectGeneric_free(ReflectGeneric *self);

uint64_t ReflectGeneric_kind(const ReflectGeneric *self);
bool ReflectGeneric_check(const ReflectGeneric *self, uint64_t typeId);
void *ReflectGeneric_call(ReflectGeneric *self, void *arg);

bool ReflectGeneric_setName(ReflectGeneric *self, const char *name);
int ReflectGeneric_getName(const ReflectGeneric *self, char *out, size_t outCap);
void ReflectGeneric_setFn(ReflectGeneric *self, ReflectGenericFn fn);
ReflectGenericFn ReflectGeneric_getFn(const ReflectGeneric *self);
void ReflectGeneric_setTarget(ReflectGeneric *self, void *target);
void *ReflectGeneric_getTarget(const ReflectGeneric *self);

void ReflectGeneric_toString(const ReflectGeneric *self, char *dest, size_t cap, bool *outTruncated);
void ReflectGeneric_toStringStruct(const ReflectGeneric *self, char *dest, size_t cap, bool *outTruncated);

#endif
