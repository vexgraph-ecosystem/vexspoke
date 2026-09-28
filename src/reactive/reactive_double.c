// reactive/reactive_double.c — the Java-double typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

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
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: the
 * 64-bit double is BIT-CAST into the engine's atomic word (never numerically
 * converted), so every bit pattern — including NaN and -0.0 — round-trips
 * exactly. One compare detects a change. Java semantics: double. Arena-allocated
 * (TYPE_REACTIVE_DOUBLE) or embedded.
 *
 * The value packing is the only per-type part: the static bitsOf/doubleOf pair is
 * the bit-cast, wired into the template's VEX_TO_WORD / VEX_FROM_WORD blanks.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveDouble (reactive/reactive_double.c)
 * ============================================================================
 * the 64-bit double typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_double.h):
 * ----------------------------------------------------------------------------
 *   ReactiveDouble { Reactive base; } // embed-first; word = bit-cast double
 *
 * PRIVATE HELPERS: bitsOf(value) / doubleOf(word) — the bit-cast pair (static)
 *
 * FUNCTION REGISTRY (emitted by the template):
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

#define VEX_NAME       ReactiveDouble
#define VEX_T          double
#define VEX_TYPE_ID    TYPE_REACTIVE_DOUBLE
#define VEX_TO_WORD(v)   bitsOf(v)
#define VEX_FROM_WORD(w) doubleOf(w)
#define VEX_ZERO       0.0
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
