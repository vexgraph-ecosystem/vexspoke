// reactive/reactive.c — the one reactive engine.

#include "reactive/reactive.h"

#include <stdatomic.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Reactive
 * ============================================================================
 * The reactive engine: one atomic word + a shadow + a dirty flag + three
 * observer lists (onSet / onChanged / onRemove). Type-agnostic — every typed
 * reactive embeds it. Reactive_set (any thread) moves the word and marks dirty
 * and never fires; Reactive_drain (owner thread) coalesces the pending writes
 * into one batch and fires onSet then onChanged, with old = shadow and new =
 * value, so a change is exactly one compare and both endpoints are exact.
 *
 * The observer lists are arena-backed, allocated lazily (an unwatched reactive
 * costs no list), and double on demand (the Dynamic Scalability & Anti-Hardcoding
 * Law). A removed observer leaves a reusable tombstone, so a mid-fire removal
 * never invalidates a fire.
 *
 * Lifetime: reactive_init for an embedded engine (inside a typed facade), or
 * Reactive_1/_2 for an arena one. reactiveShutdown fires onRemove and releases
 * the lists; reactiveFree additionally releases the block.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Reactive (reactive/reactive.c)
 * LEVEL: L2 — Behavior (reactive engine)
 * ============================================================================
 * one atomic word + shadow + dirty + three observer lists.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive.h):
 * ----------------------------------------------------------------------------
 *   Reactive {
 *     _Atomic(uintptr_t) value; // the live word
 *     uintptr_t shadow;         // owner-affine: the last drained word
 *     _Atomic bool dirty;       // a write awaits the owner's drain
 *     bool active;              // runtime-active flag
 *     ReactiveObserverList *onSet;     // fires once per drained batch
 *     ReactiveObserverList *onChanged; // fires when the drained word moved
 *     ReactiveObserverList *onRemove;  // fires on free (teardown)
 *   }
 *
 * PRIVATE HELPERS (kept file-local, pure data + list math only):
 * ----------------------------------------------------------------------------
 *   ReactiveObserver { void *cb; void *userdata; }        // SLOT RECORD
 *   ReactiveObserverList { ReactiveObserver *items; size_t count, cap; } // SLOT RECORD
 *   listEnsure(slot) / listAdd(list, cb, userdata) / listRemove(...) / listFree(list)
 *   fireSet/fireChanged/fireRemove(list, self, ...)       // the fire walks
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) Reactive_init, Reactive_1, Reactive_2, Reactive_free
 * Public Core Functions: (.h) Reactive_get, Reactive_set, Reactive_drain
 * Public Observers: (.h) Reactive_addOnSet/_addOnChanged/_addOnRemove (+ removes)
 * Public Getters: (.h) Reactive_observerCount, Reactive_isDirty, Reactive_isActive
 * ============================================================================
 */

// SLOT RECORD: one observer entry (a callback + its context).
typedef struct ReactiveObserver {
    void *cb;       // the typed callback (cast per list)
    void *userdata; // per-observer context
} ReactiveObserver;

// SLOT RECORD: one event's observer list (arena-backed, doubling).
struct ReactiveObserverList {
    ReactiveObserver *items;
    size_t count;
    size_t cap;
};

#define REACTIVE_ARRAY_TYPE (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP_REACTIVE | ID_REACTIVE)

// Mint a list on first bind (an unwatched reactive costs no list).
static ReactiveObserverList *listEnsure(ReactiveObserverList **slot) {
    if (*slot != nullptr)
        return *slot;
    ReactiveObserverList *list = (ReactiveObserverList*) Memory_alloc(REACTIVE_ARRAY_TYPE, sizeof(ReactiveObserverList));
    if (list == nullptr)
        return nullptr;
    (*list).items = nullptr;
    (*list).count = 0;
    (*list).cap = 0;
    *slot = list;
    return list;
}

// Add (or reuse a tombstone slot for) an observer. Returns false on null cb/OOM.
static bool listAdd(ReactiveObserverList **slot, void *cb, void *userdata) {
    if (cb == nullptr)
        return false;
    ReactiveObserverList *list = listEnsure(slot);
    if (list == nullptr)
        return false;
    for (size_t i = 0; i < (*list).count; i++) {
        if ((*list).items[i].cb == nullptr) { // reuse a tombstone
            (*list).items[i].cb = cb;
            (*list).items[i].userdata = userdata;
            return true;
        }
    }
    if ((*list).count >= (*list).cap) {
        size_t newCap = ((*list).cap == 0) ? 4 : (*list).cap * 2;
        ReactiveObserver *nb = (ReactiveObserver*) Memory_realloc((*list).items, newCap * sizeof(ReactiveObserver));
        if (nb == nullptr)
            return false;
        (*list).items = nb;
        (*list).cap = newCap;
    }
    (*list).items[(*list).count].cb = cb;
    (*list).items[(*list).count].userdata = userdata;
    (*list).count++;
    return true;
}

// Remove an observer (leaves a reusable tombstone). Returns false if absent.
static bool listRemove(ReactiveObserverList **slot, void *cb, void *userdata) {
    if (cb == nullptr || *slot == nullptr)
        return false;
    ReactiveObserverList *list = *slot;
    for (size_t i = 0; i < (*list).count; i++) {
        if ((*list).items[i].cb == cb && (*list).items[i].userdata == userdata) {
            (*list).items[i].cb = nullptr;
            (*list).items[i].userdata = nullptr;
            return true;
        }
    }
    return false;
}

static void listFree(ReactiveObserverList **slot) {
    if (*slot == nullptr)
        return;
    if ((**slot).items != nullptr)
        Memory_free((**slot).items);
    Memory_free(*slot);
    *slot = nullptr;
}

// CONSTRUCTORS

bool Reactive_init(Reactive *self, uintptr_t initialWord) {
    if (self == nullptr)
        return false;
    atomic_init(&(*self).value, initialWord);
    (*self).shadow = initialWord; // shadow starts equal: the first drain is silent
    atomic_init(&(*self).dirty, false);
    (*self).active = true;
    (*self).onSet = nullptr;
    (*self).onChanged = nullptr;
    (*self).onRemove = nullptr;
    return true;
}

Reactive *Reactive_1(uintptr_t initialWord) {
    Reactive *reactive = (Reactive*) Memory_alloc(REACTIVE_ARRAY_TYPE, sizeof(Reactive));
    if (reactive == nullptr)
        return nullptr;
    Reactive_init(reactive, initialWord);
    return reactive;
}

Reactive *Reactive_2(const Reactive *init, size_t count) {
    if (count == 0)
        return nullptr;
    Reactive *p = (Reactive*) Memory_alloc(TYPE_REACTIVE_ARRAY, sizeof(Reactive) * count);
    if (p == nullptr)
        return nullptr;
    uintptr_t seed = (init != nullptr) ? Reactive_get((Reactive*) init) : 0u;
    for (size_t i = 0; i < count; i++)
        Reactive_init(&p[i], seed);
    return p;
}

void Reactive_shutdown(Reactive *self) {
    if (self == nullptr)
        return;
    // Teardown notify FIRST: every onRemove observer unbinds before the lists go
    // (the Teardown Order Law — no dangling observer).
    if ((*self).onRemove != nullptr) {
        ReactiveObserverList *list = (*self).onRemove;
        for (size_t i = 0; i < (*list).count; i++) {
            ReactiveObserver o = (*list).items[i];
            if (o.cb != nullptr)
                ((ReactiveRemoveFn) o.cb)(self, o.userdata);
        }
    }
    listFree(&(*self).onSet);
    listFree(&(*self).onChanged);
    listFree(&(*self).onRemove);
    (*self).active = false;
}

void Reactive_free(Reactive *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(self);
    Memory_free(self);
}

// CORE FUNCTIONS

uintptr_t Reactive_get(Reactive *self) {
    if (self == nullptr)
        return 0u;
    return atomic_load_explicit(&(*self).value, memory_order_acquire);
}

void Reactive_set(Reactive *self, uintptr_t word) {
    if (self == nullptr)
        return;
    // Any thread: move the word, mark dirty, fire nothing.
    atomic_store_explicit(&(*self).value, word, memory_order_release);
    atomic_store_explicit(&(*self).dirty, true, memory_order_release);
}

bool Reactive_drain(Reactive *self) {
    if (self == nullptr)
        return false;
    // Consume the dirty flag first: the acq_rel exchange pairs with the writer's
    // release store, so the load below observes the written word.
    if (!atomic_exchange_explicit(&(*self).dirty, false, memory_order_acq_rel))
        return false; // nothing written since the last drain
    uintptr_t value = atomic_load_explicit(&(*self).value, memory_order_acquire);
    uintptr_t old = (*self).shadow;
    // onSet fires once for the whole batch (coalesced: last value wins).
    if ((*self).onSet != nullptr) {
        ReactiveObserverList *list = (*self).onSet;
        for (size_t i = 0; i < (*list).count; i++) {
            ReactiveObserver o = (*list).items[i];
            if (o.cb != nullptr)
                ((ReactiveSetFn) o.cb)(self, value, o.userdata);
        }
    }
    // onChanged fires only on a real move; latch the shadow before firing so a
    // re-entrant drain from a callback is a no-op.
    if (value != old) {
        (*self).shadow = value;
        if ((*self).onChanged != nullptr) {
            ReactiveObserverList *list = (*self).onChanged;
            for (size_t i = 0; i < (*list).count; i++) {
                ReactiveObserver o = (*list).items[i];
                if (o.cb != nullptr)
                    ((ReactiveChangedFn) o.cb)(self, old, value, o.userdata);
            }
        }
    }
    return true;
}

// OBSERVERS

bool Reactive_addOnSet(Reactive *self, ReactiveSetFn cb, void *userdata) {
    return self ? listAdd(&(*self).onSet, (void*) cb, userdata) : false;
}

bool Reactive_removeOnSet(Reactive *self, ReactiveSetFn cb, void *userdata) {
    return self ? listRemove(&(*self).onSet, (void*) cb, userdata) : false;
}

bool Reactive_addOnChanged(Reactive *self, ReactiveChangedFn cb, void *userdata) {
    return self ? listAdd(&(*self).onChanged, (void*) cb, userdata) : false;
}

bool Reactive_removeOnChanged(Reactive *self, ReactiveChangedFn cb, void *userdata) {
    return self ? listRemove(&(*self).onChanged, (void*) cb, userdata) : false;
}

bool Reactive_addOnRemove(Reactive *self, ReactiveRemoveFn cb, void *userdata) {
    return self ? listAdd(&(*self).onRemove, (void*) cb, userdata) : false;
}

bool Reactive_removeOnRemove(Reactive *self, ReactiveRemoveFn cb, void *userdata) {
    return self ? listRemove(&(*self).onRemove, (void*) cb, userdata) : false;
}

// GETTERS

size_t Reactive_observerCount(const Reactive *self) {
    if (self == nullptr)
        return 0u;
    size_t n = 0u;
    const ReactiveObserverList *list = (*self).onSet;
    if (list != nullptr) {
        for (size_t i = 0; i < (*list).count; i++)
            if ((*list).items[i].cb != nullptr) n++;
    }
    list = (*self).onChanged;
    if (list != nullptr) {
        for (size_t i = 0; i < (*list).count; i++)
            if ((*list).items[i].cb != nullptr) n++;
    }
    list = (*self).onRemove;
    if (list != nullptr) {
        for (size_t i = 0; i < (*list).count; i++)
            if ((*list).items[i].cb != nullptr) n++;
    }
    return n;
}

bool Reactive_isDirty(const Reactive *self) {
    if (self == nullptr)
        return false;
    return atomic_load_explicit((_Atomic bool*) &(*self).dirty, memory_order_acquire);
}

bool Reactive_isActive(const Reactive *self) {
    return self ? (*self).active : false;
}
