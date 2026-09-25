#ifndef REFLECTION_REFLECT_METHOD_H
#define REFLECTION_REFLECT_METHOD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "relational/variable_slot.h"

// reflection/reflect_method.h — the Method reflection record.
//
// One of the five reflection kinds: metadata for a stored behavior. A Method is
// a folded name (the atom's grammar), a callable taking one void* and returning
// one void*, and a target (the owner / default receiver). The kind is the header
// typeId (TYPE_REFLECT_METHOD), so ReflectMethod_check reads the identity — no
// side tag field. Block layout: [MemoryHeader 16][name 24][invoke 8][target 8].
//
// COLD PATH ONLY (the Cold-Only Reflection Law): reflection is read by the cold
// rendezvous, never on a frame.

#define REFLECT_METHOD_NAME_BYTES VARIABLE_SLOT_NAME_BYTES
#define REFLECT_METHOD_NAME_MAX VARIABLE_SLOT_NAME_MAX
#define REFLECT_METHOD_PAYLOAD_BYTES 40u

typedef void *(*ReflectMethodFn)(void *);

typedef struct ReflectMethod {
    char name[REFLECT_METHOD_NAME_BYTES]; // folded name (atom grammar)
    ReflectMethodFn invoke;               // the callable
    void *target;                         // the owner / default receiver
} ReflectMethod;

_Static_assert(sizeof(ReflectMethod) == REFLECT_METHOD_PAYLOAD_BYTES, "ReflectMethod must stay 40 bytes");

// --- Constructors ---
// Inline init: fold + validate the name, bind the callable and target. False on
// a null self or a null/empty/overlong/illegal name.
bool ReflectMethod_init(ReflectMethod *self, const char *name, ReflectMethodFn invoke, void *target);

// Arena-allocated (the Arity and Constructive Convenience Law).
ReflectMethod *ReflectMethod_0(void);
ReflectMethod *ReflectMethod_1(const char *name);
ReflectMethod *ReflectMethod_2(const char *name, ReflectMethodFn invoke);
ReflectMethod *ReflectMethod_3(const char *name, ReflectMethodFn invoke, void *target);
#define ReflectMethod(...) CONSTRUCTOR_DISPATCH(ReflectMethod, __VA_ARGS__)
void ReflectMethod_free(ReflectMethod *self);

// --- Core functions ---
uint64_t ReflectMethod_kind(const ReflectMethod *self);
bool ReflectMethod_check(const ReflectMethod *self, uint64_t typeId);
// Invoke the callable with arg (null-safe: no callable yields null).
void *ReflectMethod_call(ReflectMethod *self, void *arg);

// --- Setters / Getters (the Symmetric Getter/Setter Completeness Law) ---
bool ReflectMethod_setName(ReflectMethod *self, const char *name);
int ReflectMethod_getName(const ReflectMethod *self, char *out, size_t outCap);
void ReflectMethod_setInvoke(ReflectMethod *self, ReflectMethodFn invoke);
ReflectMethodFn ReflectMethod_getInvoke(const ReflectMethod *self);
void ReflectMethod_setTarget(ReflectMethod *self, void *target);
void *ReflectMethod_getTarget(const ReflectMethod *self);

// --- String projections (the toString Law) ---
void ReflectMethod_toString(const ReflectMethod *self, char *dest, size_t cap, bool *outTruncated);
void ReflectMethod_toStringStruct(const ReflectMethod *self, char *dest, size_t cap, bool *outTruncated);

#endif
