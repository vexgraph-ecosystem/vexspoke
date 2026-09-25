// reactive/reactive_double.c — the Java-double typed reactive.

#include "reactive/reactive_double.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveDouble
 * ============================================================================
 * A word-sized typed reactive: the 64-bit double is BIT-CAST into the engine's
 * atomic word (never numerically converted), so every bit pattern — including
 * NaN and -0.0 — round-trips exactly. One compare detects a change. Java
 * semantics: double. Arena-allocated (TYPE_REACTIVE_DOUBLE) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveDouble (reactive/reactive_double.c)
 * LEVEL: L2 — Behavior (typed reactive)
 * ============================================================================
 * the 64-bit double typed reactive (an embedded Reactive engine).
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_double.h):
 * ----------------------------------------------------------------------------
 *   ReactiveDouble { Reactive base; } // embed-first; word = bit-cast double
 *
 * PRIVATE HELPERS: bitsOf(value) / doubleOf(word) — the bit-cast pair (static)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveDouble_0(), ReactiveDouble_1(initial), _free
 * Public Setters: (.h) ReactiveDouble_set(self, value)
 * Public Getters: (.h) ReactiveDouble_get(self)
 * ============================================================================
 */

static uintptr_t bitsOf(double value) {
    uint64_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return (uintptr_t) bits;
}

static double doubleOf(uintptr_t word) {
    uint64_t bits = (uint64_t) word;
    double value = 0.0;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

ReactiveDouble *ReactiveDouble_1(double initial) {
    ReactiveDouble *self = (ReactiveDouble*) Memory_alloc(TYPE_REACTIVE_DOUBLE, sizeof(ReactiveDouble));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, bitsOf(initial));
    return self;
}

ReactiveDouble *ReactiveDouble_0(void) {
    return ReactiveDouble_1(0.0);
}

void ReactiveDouble_free(ReactiveDouble *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(&(*self).base);
    Memory_free(self);
}

void ReactiveDouble_set(ReactiveDouble *self, double value) {
    if (self == nullptr)
        return;
    Reactive_set(&(*self).base, bitsOf(value));
}

double ReactiveDouble_get(const ReactiveDouble *self) {
    if (self == nullptr)
        return 0.0;
    return doubleOf(Reactive_get((Reactive*) &(*self).base));
}
