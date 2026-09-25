#ifndef REACTIVE_REACTIVE_H
#define REACTIVE_REACTIVE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"

// reactive/reactive.h — the one reactive engine.
//
// A reactive is ONE atomic word — a scalar (<= 8 bytes) or a pointer to an
// immutable block — plus a shadow of the last drained word, a dirty flag, and
// three observer lists (onSet / onChanged / onRemove). Nothing about it is
// type-specific, so every typed reactive (reactive_int.h, reactive_string.h, …)
// EMBEDS this engine as its first member: a ReactiveInt* is also a Reactive*.
//
// WRITE ANYWHERE, NOTIFY ON THE OWNER. Reactive_set is atomic and safe from any
// thread; it moves the word and marks dirty and NEVER fires an observer. The
// owner (Thread 0, the pump / paint pass) calls Reactive_drain, which coalesces
// every write since the last drain into one batch (last value wins) and fires
// onSet (once per batch) then onChanged (only on a real move) on the owner's
// thread. This is the Present-On-Demand Law applied to data: a background writer
// just moves the value; the owner decides when to hear about it. Drain never
// blocks (the Bounded Wait Law).
//
// THE CHANGE RULE: a value is "different from before" when the drained word
// differs from the shadow — exactly one compare, and old + new are both exact
// (the shadow IS the old value). That is why the unit is the word (a field), not
// a struct: a word is atomic, a struct is not.

typedef struct ReactiveObserverList ReactiveObserverList;

typedef struct Reactive {
    _Atomic(uintptr_t) value;  // the live word (scalar, or ptr to an immutable block)
    uintptr_t shadow;          // owner-affine: the last drained word
    _Atomic bool dirty;        // a write awaits the owner's drain
    bool active;               // runtime-active flag
    ReactiveObserverList *onSet;     // fires once per drained batch
    ReactiveObserverList *onChanged; // fires when the drained word moved
    ReactiveObserverList *onRemove;  // fires on free (teardown)
} Reactive;

typedef void (*ReactiveSetFn)(Reactive *self, uintptr_t newValue, void *userdata);
typedef void (*ReactiveChangedFn)(Reactive *self, uintptr_t oldValue, uintptr_t newValue, void *userdata);
typedef void (*ReactiveRemoveFn)(Reactive *self, void *userdata);

// --- Constructors ---
// Embedded init: seed the word (its own shadow starts equal, so the first drain
// is silent). The typed reactives call this from their constructors.
bool Reactive_init(Reactive *self, uintptr_t initialWord);
// Arena-allocated conveniences (the Arity and Constructive Convenience Law).
Reactive *Reactive_1(uintptr_t initialWord);
Reactive *Reactive_2(const Reactive *init, size_t count);
#define Reactive(...) CONSTRUCTOR_DISPATCH(Reactive, __VA_ARGS__)
// Fire onRemove and release the observer lists, WITHOUT freeing the block
// (embedded engines live inside a typed facade, which frees itself).
void Reactive_shutdown(Reactive *self);
void Reactive_free(Reactive *self);

// --- Core functions ---
// Atomic acquire load — safe from any thread.
uintptr_t Reactive_get(Reactive *self);
// Atomic release store + dirty mark. Safe from ANY thread and NEVER fires
// observers. For a word-sized reactive the word IS the value; for a big one it
// is a pointer to an immutable block (see the typed facades).
void Reactive_set(Reactive *self, uintptr_t word);
// The owner-thread notification point: consume the writes since the last drain
// as ONE coalesced batch and fire onSet then onChanged (only on a real move).
// Returns true when a batch fired. Lock-free — never blocks.
bool Reactive_drain(Reactive *self);

// --- Observer add / remove (per event; multiple functions allowed) ---
// Owner-affine: add/remove on the owner thread only.
bool Reactive_addOnSet(Reactive *self, ReactiveSetFn cb, void *userdata);
bool Reactive_removeOnSet(Reactive *self, ReactiveSetFn cb, void *userdata);
bool Reactive_addOnChanged(Reactive *self, ReactiveChangedFn cb, void *userdata);
bool Reactive_removeOnChanged(Reactive *self, ReactiveChangedFn cb, void *userdata);
bool Reactive_addOnRemove(Reactive *self, ReactiveRemoveFn cb, void *userdata);
bool Reactive_removeOnRemove(Reactive *self, ReactiveRemoveFn cb, void *userdata);

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
size_t Reactive_observerCount(const Reactive *self);
bool Reactive_isDirty(const Reactive *self);
bool Reactive_isActive(const Reactive *self);

#endif
