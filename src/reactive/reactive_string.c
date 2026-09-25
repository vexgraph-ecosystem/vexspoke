// reactive/reactive_string.c — the Java-String typed reactive.

#include "reactive/reactive_string.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveString
 * ============================================================================
 * A pointer-word typed reactive: the engine's atomic word holds a pointer to a
 * string block. A rebind (a new pointer) is a change; in-place content mutation
 * behind the pointer is invisible (publish a fresh block to signal). Java
 * semantics: an immutable String reference. Arena-allocated
 * (TYPE_REACTIVE_STRING) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveString (reactive/reactive_string.c)
 * LEVEL: L2 — Behavior (typed reactive)
 * ============================================================================
 * the String typed reactive (an embedded Reactive engine; word = a block ptr).
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_string.h):
 * ----------------------------------------------------------------------------
 *   ReactiveString { Reactive base; } // embed-first; word = uint8_t* block
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveString_0(), ReactiveString_1(initial), _free
 * Public Setters: (.h) ReactiveString_set(self, value)
 * Public Getters: (.h) ReactiveString_get(self)
 * ============================================================================
 */

ReactiveString *ReactiveString_1(const uint8_t *initial) {
    ReactiveString *self = (ReactiveString*) Memory_alloc(TYPE_REACTIVE_STRING, sizeof(ReactiveString));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, (uintptr_t) initial);
    return self;
}

ReactiveString *ReactiveString_0(void) {
    return ReactiveString_1(nullptr);
}

void ReactiveString_free(ReactiveString *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(&(*self).base);
    Memory_free(self);
}

void ReactiveString_set(ReactiveString *self, const uint8_t *value) {
    if (self == nullptr)
        return;
    Reactive_set(&(*self).base, (uintptr_t) value);
}

const uint8_t *ReactiveString_get(const ReactiveString *self) {
    if (self == nullptr)
        return nullptr;
    return (const uint8_t*) Reactive_get((Reactive*) &(*self).base);
}
