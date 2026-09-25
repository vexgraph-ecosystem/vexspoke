#ifndef REFLECTION_REFLECT_FIELD_H
#define REFLECTION_REFLECT_FIELD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "relational/variable_slot.h"

// reflection/reflect_field.h — the Field reflection record.
//
// The Field kind of reflection: metadata for a member slot that carries TWO
// behaviors — a getter (void* (*)(void*)) and a setter (void (*)(void*, void*))
// — plus a folded name and a target (the field's owner / address). The kind is
// the header typeId (TYPE_REFLECT_FIELD). Block layout:
// [MemoryHeader 16][name 24][get 8][set 8][target 8] = 64 bytes.

#define REFLECT_FIELD_NAME_BYTES VARIABLE_SLOT_NAME_BYTES
#define REFLECT_FIELD_NAME_MAX VARIABLE_SLOT_NAME_MAX
#define REFLECT_FIELD_PAYLOAD_BYTES 48u

typedef void *(*ReflectFieldGetFn)(void *);
typedef void (*ReflectFieldSetFn)(void *, void *);

typedef struct ReflectField {
    char name[REFLECT_FIELD_NAME_BYTES]; // folded name (atom grammar)
    ReflectFieldGetFn get;               // getter
    ReflectFieldSetFn set;               // setter
    void *target;                        // the field's owner / address
} ReflectField;

_Static_assert(sizeof(ReflectField) == REFLECT_FIELD_PAYLOAD_BYTES, "ReflectField must stay 48 bytes");

bool ReflectField_init(ReflectField *self, const char *name, ReflectFieldGetFn get, ReflectFieldSetFn set, void *target);

ReflectField *ReflectField_0(void);
ReflectField *ReflectField_1(const char *name);
ReflectField *ReflectField_2(const char *name, ReflectFieldGetFn get);
ReflectField *ReflectField_3(const char *name, ReflectFieldGetFn get, ReflectFieldSetFn set);
ReflectField *ReflectField_4(const char *name, ReflectFieldGetFn get, ReflectFieldSetFn set, void *target);
#define ReflectField(...) CONSTRUCTOR_DISPATCH(ReflectField, __VA_ARGS__)
void ReflectField_free(ReflectField *self);

uint64_t ReflectField_kind(const ReflectField *self);
bool ReflectField_check(const ReflectField *self, uint64_t typeId);
// Read the field from a receiver (null-safe: no getter yields null).
void *ReflectField_read(ReflectField *self, void *receiver);
// Write the field on a receiver (null-safe: no setter is a no-op).
void ReflectField_write(ReflectField *self, void *receiver, void *value);

bool ReflectField_setName(ReflectField *self, const char *name);
int ReflectField_getName(const ReflectField *self, char *out, size_t outCap);
void ReflectField_setGet(ReflectField *self, ReflectFieldGetFn get);
ReflectFieldGetFn ReflectField_getGet(const ReflectField *self);
void ReflectField_setSet(ReflectField *self, ReflectFieldSetFn set);
ReflectFieldSetFn ReflectField_getSet(const ReflectField *self);
void ReflectField_setTarget(ReflectField *self, void *target);
void *ReflectField_getTarget(const ReflectField *self);

void ReflectField_toString(const ReflectField *self, char *dest, size_t cap, bool *outTruncated);
void ReflectField_toStringStruct(const ReflectField *self, char *dest, size_t cap, bool *outTruncated);

#endif
