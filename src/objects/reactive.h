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
//   onSet      fires once per drained batch of writes
//   onChanged  fires when the drained value actually moved (old vs new)
//   onRemove   fires on Reactive_free (teardown), so every observer unbinds
//              BEFORE the memory goes (the Teardown Order Law — no dangling)
//
// ATOMIC PAYLOAD, OWNER-AFFINE NOTIFICATION. The payload is atomic
// (acquire/release) and every write marks an atomic dirty flag, so Reactive_set
// is safe from ANY thread and NEVER fires observers on the writer's thread. The
// owner (Thread 0 — the pump / paint pass) calls Reactive_drain, which coalesces
// every write since the last drain into ONE notification batch and fires
// onSet/onChanged on the owner's thread. Observers therefore always run on the
// owner, and the observer lists themselves stay owner-affine (add/remove on the
// owner only). This is the Present-On-Demand Law applied to data: a background
// writer just moves the value; the owner decides when to hear about it.
//
// Observers are added and removed (never "set"), so N functions bind one
// Reactive, each with its own userdata. Callbacks receive the Reactive itself as
// their first argument, so one observer bound to several reactives knows which
// one fired. Arena-allocated.

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
// Atomic acquire load — safe from any thread.
uint64_t Reactive_get(Reactive *reactive);

// Atomic release store + dirty mark. Safe from ANY thread and NEVER fires
// observers — a cross-thread writer only moves the value (the owner fires on
// the next drain).
void Reactive_set(Reactive *reactive, uint64_t value);

// The owner-thread notification point: consume the writes since the last drain
// as ONE coalesced batch (last value wins) and fire onSet (once per batch) then
// onChanged (only when the value moved). Returns true when a batch fired.
// Lock-free — never blocks (the Bounded Wait Law).
bool Reactive_drain(Reactive *reactive);

// --- Observer add / remove (per event; multiple functions allowed) ---
// Owner-affine: add/remove on the owner thread only.
bool Reactive_addOnSet(Reactive *reactive, ReactiveSetFn cb, void *userdata);
bool Reactive_removeOnSet(Reactive *reactive, ReactiveSetFn cb, void *userdata);
bool Reactive_addOnChanged(Reactive *reactive, ReactiveChangedFn cb, void *userdata);
bool Reactive_removeOnChanged(Reactive *reactive, ReactiveChangedFn cb, void *userdata);
bool Reactive_addOnRemove(Reactive *reactive, ReactiveRemoveFn cb, void *userdata);
bool Reactive_removeOnRemove(Reactive *reactive, ReactiveRemoveFn cb, void *userdata);

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
size_t Reactive_observerCount(const Reactive *reactive);   // all three lists
bool   Reactive_isDirty(const Reactive *reactive);         // a write awaits drain

#endif // OBJECTS_REACTIVE_H
