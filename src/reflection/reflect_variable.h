#ifndef REFLECTION_REFLECT_VARIABLE_H
#define REFLECTION_REFLECT_VARIABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "relational/variable_slot.h"

// reflection/reflect_variable.h — the Variable reflection record.
//
// The Variable kind of reflection: metadata for a live symbol — a folded name
// (the atom's grammar), a reader (void* (*)(void*)), and a target (the value /
// owner). The kind is the header typeId (TYPE_REFLECT_VARIABLE). Block layout:
// [MemoryHeader 16][name 24][read 8][target 8] = 56 bytes.

#define REFLECT_VARIABLE_NAME_BYTES VARIABLE_SLOT_NAME_BYTES
#define REFLECT_VARIABLE_NAME_MAX VARIABLE_SLOT_NAME_MAX
#define REFLECT_VARIABLE_PAYLOAD_BYTES 40u

typedef void *(*ReflectVariableReadFn)(void *);

typedef struct ReflectVariable {
    char name[REFLECT_VARIABLE_NAME_BYTES]; // folded name (atom grammar)
    ReflectVariableReadFn read;             // the reader
    void *target;                           // the value / owner
} ReflectVariable;

_Static_assert(sizeof(ReflectVariable) == REFLECT_VARIABLE_PAYLOAD_BYTES, "ReflectVariable must stay 40 bytes");

bool ReflectVariable_init(ReflectVariable *self, const char *name, ReflectVariableReadFn read, void *target);

ReflectVariable *ReflectVariable_0(void);
ReflectVariable *ReflectVariable_1(const char *name);
ReflectVariable *ReflectVariable_2(const char *name, ReflectVariableReadFn read);
ReflectVariable *ReflectVariable_3(const char *name, ReflectVariableReadFn read, void *target);
#define ReflectVariable(...) CONSTRUCTOR_DISPATCH(ReflectVariable, __VA_ARGS__)
void ReflectVariable_free(ReflectVariable *self);

uint64_t ReflectVariable_kind(const ReflectVariable *self);
bool ReflectVariable_check(const ReflectVariable *self, uint64_t typeId);
void *ReflectVariable_read(ReflectVariable *self, void *arg);

bool ReflectVariable_setName(ReflectVariable *self, const char *name);
int ReflectVariable_getName(const ReflectVariable *self, char *out, size_t outCap);
void ReflectVariable_setRead(ReflectVariable *self, ReflectVariableReadFn read);
ReflectVariableReadFn ReflectVariable_getRead(const ReflectVariable *self);
void ReflectVariable_setTarget(ReflectVariable *self, void *target);
void *ReflectVariable_getTarget(const ReflectVariable *self);

void ReflectVariable_toString(const ReflectVariable *self, char *dest, size_t cap, bool *outTruncated);
void ReflectVariable_toStringStruct(const ReflectVariable *self, char *dest, size_t cap, bool *outTruncated);

#endif
