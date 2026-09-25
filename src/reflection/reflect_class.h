#ifndef REFLECTION_REFLECT_CLASS_H
#define REFLECTION_REFLECT_CLASS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "relational/variable_slot.h"

// reflection/reflect_class.h — the Class reflection record.
//
// The Class kind of reflection: metadata for a type — a folded name (the atom's
// grammar), a constructor (void* (*)(void*)), and a schema pointer (the class
// descriptor). The kind is the header typeId (TYPE_REFLECT_CLASS). Block layout:
// [MemoryHeader 16][name 24][construct 8][schema 8] = 56 bytes.

#define REFLECT_CLASS_NAME_BYTES VARIABLE_SLOT_NAME_BYTES
#define REFLECT_CLASS_NAME_MAX VARIABLE_SLOT_NAME_MAX
#define REFLECT_CLASS_PAYLOAD_BYTES 40u

typedef void *(*ReflectClassConstructFn)(void *);

typedef struct ReflectClass {
    char name[REFLECT_CLASS_NAME_BYTES]; // folded name (atom grammar)
    ReflectClassConstructFn construct;   // the constructor
    void *schema;                        // the class descriptor
} ReflectClass;

_Static_assert(sizeof(ReflectClass) == REFLECT_CLASS_PAYLOAD_BYTES, "ReflectClass must stay 40 bytes");

bool ReflectClass_init(ReflectClass *self, const char *name, ReflectClassConstructFn construct, void *schema);

ReflectClass *ReflectClass_0(void);
ReflectClass *ReflectClass_1(const char *name);
ReflectClass *ReflectClass_2(const char *name, ReflectClassConstructFn construct);
ReflectClass *ReflectClass_3(const char *name, ReflectClassConstructFn construct, void *schema);
#define ReflectClass(...) CONSTRUCTOR_DISPATCH(ReflectClass, __VA_ARGS__)
void ReflectClass_free(ReflectClass *self);

uint64_t ReflectClass_kind(const ReflectClass *self);
bool ReflectClass_check(const ReflectClass *self, uint64_t typeId);
void *ReflectClass_construct(ReflectClass *self, void *arg);

bool ReflectClass_setName(ReflectClass *self, const char *name);
int ReflectClass_getName(const ReflectClass *self, char *out, size_t outCap);
void ReflectClass_setConstruct(ReflectClass *self, ReflectClassConstructFn construct);
ReflectClassConstructFn ReflectClass_getConstruct(const ReflectClass *self);
void ReflectClass_setSchema(ReflectClass *self, void *schema);
void *ReflectClass_getSchema(const ReflectClass *self);

void ReflectClass_toString(const ReflectClass *self, char *dest, size_t cap, bool *outTruncated);
void ReflectClass_toStringStruct(const ReflectClass *self, char *dest, size_t cap, bool *outTruncated);

#endif
