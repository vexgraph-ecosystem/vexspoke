// reactive/reactive_int.c — the Java-int typed reactive.

#include "reactive/reactive_int.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveInt
 * ============================================================================
 * A word-sized typed reactive: it embeds the engine and the engine's atomic word
 * IS the 32-bit signed value (sign-extended on read). One compare detects a
 * change. Java semantics: int. Arena-allocated (TYPE_REACTIVE_INT) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveInt (reactive/reactive_int.c)
 * LEVEL: L2 — Behavior (typed reactive)
 * ============================================================================
 * the 32-bit signed typed reactive (an embedded Reactive engine).
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_int.h):
 * ----------------------------------------------------------------------------
 *   ReactiveInt { Reactive base; } // embed-first; word = int32
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveInt_0(), ReactiveInt_1(initial), _free
 * Public Setters: (.h) ReactiveInt_set(self, value)
 * Public Getters: (.h) ReactiveInt_get(self)
 * ============================================================================
 */

ReactiveInt *ReactiveInt_1(int32_t initial) {
    ReactiveInt *self = (ReactiveInt*) Memory_alloc(TYPE_REACTIVE_INT, sizeof(ReactiveInt));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, (uintptr_t) (uint32_t) initial);
    return self;
}

ReactiveInt *ReactiveInt_0(void) {
    return ReactiveInt_1(0);
}

void ReactiveInt_free(ReactiveInt *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(&(*self).base);
    Memory_free(self);
}

void ReactiveInt_set(ReactiveInt *self, int32_t value) {
    if (self == nullptr)
        return;
    Reactive_set(&(*self).base, (uintptr_t) (uint32_t) value);
}

int32_t ReactiveInt_get(const ReactiveInt *self) {
    if (self == nullptr)
        return 0;
    return (int32_t) (uint32_t) Reactive_get((Reactive*) &(*self).base);
}
