#include "objects/reactive.h"

#include <string.h>

#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Reactive
 * ============================================================================
 * The per-variable event emitter: a uint64 payload plus THREE observer lists —
 * onSet (every write), onChanged (a real value change, old vs new), onRemove
 * (fired on Reactive_free so every observer unbinds before the memory goes, the
 * Teardown Order Law). Observers are added and removed (never "set"), so N
 * functions bind one reactive, each with its own userdata, and each callback
 * receives the reactive as its first argument.
 *
 * Reads and writes are plain (non-atomic): single-threaded event-driven UI
 * state. The lists are arena-backed and double on demand (the Dynamic
 * Scalability & Anti-Hardcoding Law); a removed observer leaves a reusable
 * tombstone so a fire is never invalidated by a mid-fire removal.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Reactive (objects/reactive.c)
 * LEVEL: L2 — Behavior (per-variable event emitter)
 * ============================================================================
 * SUMMARY:
 *   uint64 payload + three observer lists (onSet/onChanged/onRemove). add/remove
 *   observers; set fires onSet (all) then onChanged (all, on a real move); free
 *   fires onRemove (all) then releases.
 *
 * STRUCT FIELDS (local to this file):
 * ----------------------------------------------------------------------------
 *   Reactive {
 *     uint64_t value;            // current payload
 *     ObserverList onSet;        // fires on write
 *     ObserverList onChanged;    // fires when the value changes
 *     ObserverList onRemove;     // fires on free (teardown)
 *   }
 *   Observer     { void *cb; void *userdata; }          // SLOT RECORD
 *   ObserverList { Observer *items; size_t count, cap; } // SLOT RECORD
 *
 * PRIVATE HELPERS:
 * ----------------------------------------------------------------------------
 *   listAdd(list, cb, userdata) / listRemove(list, cb, userdata) / listFree(list)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - Reactive_1(initialValue) / Reactive_2(init, count) / Reactive_free(reactive)
 *
 * Public Core Functions: (.h)
 *   - Reactive_get(reactive) / Reactive_set(reactive, value)
 *   - Reactive_addOnSet/OnChanged/OnRemove(reactive, cb, userdata)
 *   - Reactive_removeOnSet/OnChanged/OnRemove(reactive, cb, userdata)
 *
 * Public Getters: (.h)
 *   - Reactive_observerCount(reactive)
 * ============================================================================
 */

// SLOT RECORD: one observer entry (a callback + its context).
typedef struct Observer {
    void *cb;         // the typed callback (cast per list)
    void *userdata;   // per-observer context
} Observer;

// SLOT RECORD: one event's observer list (arena-backed, doubling).
typedef struct ObserverList {
    Observer *items;
    size_t count;
    size_t cap;
} ObserverList;

struct Reactive {
    uint64_t value;
    ObserverList onSet;
    ObserverList onChanged;
    ObserverList onRemove;
};

#define REACTIVE_ARRAY_TYPE (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP_REACTIVE | ID_REACTIVE)

// Add (or reuse a tombstone slot for) an observer. Returns false on null cb/OOM.
static bool listAdd(ObserverList *list, void *cb, void *userdata) {
    if (cb == nullptr)
        return false;
    for (size_t i = 0; i < (*list).count; i++) {
        if ((*list).items[i].cb == nullptr) {   // reuse a tombstone
            (*list).items[i].cb = cb;
            (*list).items[i].userdata = userdata;
            return true;
        }
    }
    if ((*list).count >= (*list).cap) {
        size_t newCap = ((*list).cap == 0) ? 4 : (*list).cap * 2;
        Observer *nb = (Observer*) Memory_realloc((*list).items, newCap * sizeof(Observer));
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
static bool listRemove(ObserverList *list, void *cb, void *userdata) {
    if (cb == nullptr)
        return false;
    for (size_t i = 0; i < (*list).count; i++) {
        if ((*list).items[i].cb == cb && (*list).items[i].userdata == userdata) {
            (*list).items[i].cb = nullptr;
            (*list).items[i].userdata = nullptr;
            return true;
        }
    }
    return false;
}

static void listFree(ObserverList *list) {
    if ((*list).items != nullptr)
        Memory_free((*list).items);
    (*list).items = nullptr;
    (*list).count = 0;
    (*list).cap = 0;
}

// CONSTRUCTORS (PUBLIC & PRIVATE)

Reactive *Reactive_1(uint64_t initialValue) {
    uint64_t type = REACTIVE_ARRAY_TYPE;
    Reactive *reactive = (Reactive*) Memory_alloc(type, sizeof(Reactive));
    if (!reactive)
        return nullptr;
    memset(reactive, 0, sizeof(Reactive));
    (*reactive).value = initialValue;
    return reactive;
}

Reactive *Reactive_2(const Reactive *init, size_t count) {
    if (count == 0)
        return nullptr;
    Reactive *p = (Reactive*) Memory_alloc(TYPE_REACTIVE_ARRAY, sizeof(Reactive) * count);
    if (!p)
        return nullptr;
    for (size_t i = 0; i < count; i++) {
        p[i] = init ? *init : (Reactive){0};
        p[i].onSet.items = nullptr; p[i].onSet.count = 0; p[i].onSet.cap = 0;
        p[i].onChanged.items = nullptr; p[i].onChanged.count = 0; p[i].onChanged.cap = 0;
        p[i].onRemove.items = nullptr; p[i].onRemove.count = 0; p[i].onRemove.cap = 0;
    }
    return p;
}

void Reactive_free(Reactive *reactive) {
    if (!reactive)
        return;
    // Teardown notify FIRST: every onRemove observer unbinds before the memory
    // goes (the Teardown Order Law — no dangling observer).
    for (size_t i = 0; i < (*reactive).onRemove.count; i++) {
        Observer o = (*reactive).onRemove.items[i];
        if (o.cb != nullptr)
            ((ReactiveRemoveFn) o.cb)(reactive, o.userdata);
    }
    listFree(&(*reactive).onSet);
    listFree(&(*reactive).onChanged);
    listFree(&(*reactive).onRemove);
    Memory_free(reactive);
}

// CORE FUNCTIONS (PUBLIC & PRIVATE)

uint64_t Reactive_get(Reactive *reactive) {
    if (!reactive)
        return 0;
    return (*reactive).value;
}

void Reactive_set(Reactive *reactive, uint64_t value) {
    if (!reactive)
        return;
    uint64_t oldValue = (*reactive).value;
    (*reactive).value = value;
    // onSet fires on EVERY write (copy each observer — a callback may mutate the list).
    for (size_t i = 0; i < (*reactive).onSet.count; i++) {
        Observer o = (*reactive).onSet.items[i];
        if (o.cb != nullptr)
            ((ReactiveSetFn) o.cb)(reactive, value, o.userdata);
    }
    // onChanged fires only on a real move.
    if (oldValue != value) {
        for (size_t i = 0; i < (*reactive).onChanged.count; i++) {
            Observer o = (*reactive).onChanged.items[i];
            if (o.cb != nullptr)
                ((ReactiveChangedFn) o.cb)(reactive, oldValue, value, o.userdata);
        }
    }
}

bool Reactive_addOnSet(Reactive *reactive, ReactiveSetFn cb, void *userdata) {
    return reactive ? listAdd(&(*reactive).onSet, (void*) cb, userdata) : false;
}

bool Reactive_removeOnSet(Reactive *reactive, ReactiveSetFn cb, void *userdata) {
    return reactive ? listRemove(&(*reactive).onSet, (void*) cb, userdata) : false;
}

bool Reactive_addOnChanged(Reactive *reactive, ReactiveChangedFn cb, void *userdata) {
    return reactive ? listAdd(&(*reactive).onChanged, (void*) cb, userdata) : false;
}

bool Reactive_removeOnChanged(Reactive *reactive, ReactiveChangedFn cb, void *userdata) {
    return reactive ? listRemove(&(*reactive).onChanged, (void*) cb, userdata) : false;
}

bool Reactive_addOnRemove(Reactive *reactive, ReactiveRemoveFn cb, void *userdata) {
    return reactive ? listAdd(&(*reactive).onRemove, (void*) cb, userdata) : false;
}

bool Reactive_removeOnRemove(Reactive *reactive, ReactiveRemoveFn cb, void *userdata) {
    return reactive ? listRemove(&(*reactive).onRemove, (void*) cb, userdata) : false;
}

size_t Reactive_observerCount(const Reactive *reactive) {
    if (reactive == nullptr)
        return 0;
    size_t n = 0;
    for (size_t i = 0; i < (*reactive).onSet.count; i++)
        if ((*reactive).onSet.items[i].cb != nullptr) n++;
    for (size_t i = 0; i < (*reactive).onChanged.count; i++)
        if ((*reactive).onChanged.items[i].cb != nullptr) n++;
    for (size_t i = 0; i < (*reactive).onRemove.count; i++)
        if ((*reactive).onRemove.items[i].cb != nullptr) n++;
    return n;
}
