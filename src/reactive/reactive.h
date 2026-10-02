#ifndef REACTIVE_REACTIVE_H
#define REACTIVE_REACTIVE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// reactive/reactive.h — the one reactive engine.
//
// A reactive is ONE atomic word — a scalar (<= 8 bytes) or a pointer to an
// immutable block — plus a shadow of the last drained word, a dirty flag, and
// four observer lists (onSet / onChanged / onGet / onNullptr). Nothing about it is
// type-specific, so every typed reactive (reactive_int.h, reactive_string.h, …)
// EMBEDS this engine as its first member: a ReactiveInt* is also a Reactive*.
//
// WRITE ANYWHERE, NOTIFY ON THE OWNER. Reactive_store is atomic and safe from any
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
typedef struct Reactive Reactive;

// The delivery hook: called at the very END of Reactive_store, on the WRITER's
// thread. It must NOT fire observers (that stays in Reactive_drain) — it only
// hands the box to an owner's queue (thread/reactive.c) so the owner drains it.
// nullptr = no auto-delivery: the caller or owner drains by hand as before.
typedef void (*ReactiveDeliverFn)(Reactive *self);

// The value stringifier: formats the raw word as text FOR ITS TYPE. Bound by each
// typed reactive (int -> "%d", float -> "%g", string -> the bytes, …) so a
// type-erased consumer (a Label's [] slot) can render any reactive as text — a
// valueOf() in the reactive section.
typedef void (*ReactiveValueFn)(uintptr_t word, char *out, size_t cap);

struct Reactive {
    _Atomic(uintptr_t) value;  // the live word (scalar, or ptr to an immutable block)
    uintptr_t shadow;          // owner-affine: the last drained word
    _Atomic bool dirty;        // a write awaits the owner's drain
    bool active;               // runtime-active flag
    bool deliverBound;         // true: use `deliver` verbatim; false: inherit the default
    ReactiveDeliverFn deliver; // the box's own hook (nullptr = none, when bound)
    ReactiveValueFn valueOf;   // formats the word as text (typed), or nullptr
    ReactiveObserverList *onSet;     // fires once per drained batch
    ReactiveObserverList *onChanged; // fires when the drained word moved
    ReactiveObserverList *onGet;     // fires on each read (immediate; the reader's thread)
    ReactiveObserverList *onNullptr; // fires when a drained word is 0 (empty)
};

typedef void (*ReactiveSetFn)(Reactive *self, uintptr_t newValue, void *userdata);
typedef void (*ReactiveChangedFn)(Reactive *self, uintptr_t oldValue, uintptr_t newValue, void *userdata);
typedef void (*ReactiveGetFn)(Reactive *self, uintptr_t value, void *userdata);
typedef void (*ReactiveNullptrFn)(Reactive *self, void *userdata);

// --- Constructors ---
// Embedded init: seed the word (its own shadow starts equal, so the first drain
// is silent). The typed reactives call this from their constructors.
bool Reactive_init(Reactive *self, uintptr_t initialWord);
// Arena-allocated conveniences (the Arity and Constructive Convenience Law).
Reactive *Reactive_1(uintptr_t initialWord);
Reactive *Reactive_2(const Reactive *init, size_t count);
// The bare `Reactive(...)` arity chooser is retired: the token is promoted to the
// generic family constructor `Reactive(T)` (reactive/generic.h), so a bare engine
// is built through Reactive_1/Reactive_2, and typed reactives through Reactive(T).
// Release the observer lists, WITHOUT freeing the block (embedded engines live
// inside a typed facade, which frees itself). Freeing is just freeing.
void Reactive_shutdown(Reactive *self);
void Reactive_free(Reactive *self);

// --- Core functions ---
// Atomic acquire load — safe from any thread. Fires the onGet observers
// immediately (on the caller's thread) when any are bound; with none bound it is
// a single atomic load, so a bare read stays hot-path cheap.
uintptr_t Reactive_load(Reactive *self);
// Atomic release store + dirty mark. Safe from ANY thread and NEVER fires
// observers. For a word-sized reactive the word IS the value; for a big one it
// is a pointer to an immutable block (see the typed facades). When a delivery
// hook is bound (below), store ENDS by calling it — so `set` alone can schedule
// the owner's drain, without the caller ever naming Reactive_drain.
void Reactive_store(Reactive *self, uintptr_t word);

// Bind the auto-delivery hook (or nullptr to clear). Owner-affine: bind before
// producers run. thread/reactive.c binds its wake here so an adopted box fires
// its observers on the reactive thread after a plain set.
void Reactive_setDelivery(Reactive *self, ReactiveDeliverFn deliver);
// Drop the box's own hook so it INHERITS the process default (see below).
void Reactive_clearDelivery(Reactive *self);
ReactiveDeliverFn Reactive_getDelivery(const Reactive *self);

// The process-wide default delivery hook: used by every box that has not bound
// its own (deliverBound == false). thread/reactive.c sets this to its wake, so
// a plain Reactive_store on ANY box schedules the reactive thread's drain — set
// is the only verb, drain is never called by hand. nullptr restores manual drain.
void Reactive_setDefaultDelivery(ReactiveDeliverFn deliver);
ReactiveDeliverFn Reactive_getDefaultDelivery(void);

// Bind the box's value stringifier (typed), or nullptr to fall back to the hex
// word. The typed constructors bind their own; a hand-built engine may set one.
void Reactive_setValueOf(Reactive *self, ReactiveValueFn valueOf);
// Format the current value into `out` (nul-terminated within cap) and return it.
// Falls back to "%llu" of the raw word when no stringifier is bound.
const char *Reactive_valueOf(Reactive *self, char *out, size_t cap);
// The owner-thread notification point: consume the writes since the last drain
// as ONE coalesced batch and fire onSet then onChanged (only on a real move).
// Returns true when a batch fired. Lock-free — never blocks.
bool Reactive_drain(Reactive *self);

// --- Observer add / remove (per event; multiple functions allowed) ---
// Owner-affine: add/remove on the owner thread only.
bool Reactive_watchSet(Reactive *self, ReactiveSetFn cb, void *userdata);
bool Reactive_unwatchSet(Reactive *self, ReactiveSetFn cb, void *userdata);
bool Reactive_watchChanged(Reactive *self, ReactiveChangedFn cb, void *userdata);
bool Reactive_unwatchChanged(Reactive *self, ReactiveChangedFn cb, void *userdata);
bool Reactive_watchGet(Reactive *self, ReactiveGetFn cb, void *userdata);
bool Reactive_unwatchGet(Reactive *self, ReactiveGetFn cb, void *userdata);
bool Reactive_watchNullptr(Reactive *self, ReactiveNullptrFn cb, void *userdata);
bool Reactive_unwatchNullptr(Reactive *self, ReactiveNullptrFn cb, void *userdata);

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
size_t Reactive_observerCount(const Reactive *self);
bool Reactive_isDirty(const Reactive *self);
bool Reactive_isActive(const Reactive *self);

#endif
