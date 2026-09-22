#ifndef OBJECTS_REACTIVE_H
#define OBJECTS_REACTIVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"

// objects/reactive.h — the per-variable event emitter.
//
// A Reactive wraps a uint64 payload and THREE observer lists — an event bus at
// the variable level, not a single callback slot:
//   onSet      fires on every write
//   onChanged  fires only when the value actually changes (old vs new)
//   onRemove   fires on Reactive_free (teardown), so every observer unbinds
//              BEFORE the memory goes (the Teardown Order Law — no dangling)
//
// Observers are added and removed (never "set"), so N functions bind one
// Reactive, each with its own userdata. Callbacks receive the Reactive itself as
// their first argument, so one observer bound to several reactives knows which
// one fired.
//
// Reads and writes are plain (non-atomic) by design: single-threaded
// event-driven UI state, not cross-thread signaling. Arena-allocated.

typedef struct Reactive Reactive;

typedef void (*ReactiveSetFn)(Reactive *self, uint64_t newValue, void *userdata);
typedef void (*ReactiveChangedFn)(Reactive *self, uint64_t oldValue, uint64_t newValue, void *userdata);
typedef void (*ReactiveRemoveFn)(Reactive *self, void *userdata);

// --- Constructors ---
Reactive *Reactive_1(uint64_t initialValue);
Reactive *Reactive_2(const Reactive *init, size_t count);

#define Reactive(...) CONSTRUCTOR_DISPATCH(Reactive, __VA_ARGS__)

// Free the reactive (fires onRemove first, then releases the observer lists).
void Reactive_free(Reactive *reactive);

// --- Core functions ---
uint64_t Reactive_get(Reactive *reactive);
void     Reactive_set(Reactive *reactive, uint64_t value);

// --- Observer add / remove (per event; multiple functions allowed) ---
bool Reactive_addOnSet(Reactive *reactive, ReactiveSetFn cb, void *userdata);
bool Reactive_removeOnSet(Reactive *reactive, ReactiveSetFn cb, void *userdata);
bool Reactive_addOnChanged(Reactive *reactive, ReactiveChangedFn cb, void *userdata);
bool Reactive_removeOnChanged(Reactive *reactive, ReactiveChangedFn cb, void *userdata);
bool Reactive_addOnRemove(Reactive *reactive, ReactiveRemoveFn cb, void *userdata);
bool Reactive_removeOnRemove(Reactive *reactive, ReactiveRemoveFn cb, void *userdata);

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
size_t Reactive_observerCount(const Reactive *reactive);   // all three lists

#endif // OBJECTS_REACTIVE_H
