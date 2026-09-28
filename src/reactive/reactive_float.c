// reactive/reactive_float.c — the Java-float typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_float.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveFloat
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: the
 * 32-bit float is BIT-CAST into the engine's atomic word (never numerically
 * converted), so every 32-bit pattern — including NaN and -0.0 — round-trips
 * exactly. Java semantics: float. Arena-allocated (TYPE_REACTIVE_FLOAT) or
 * embedded.
 *
 * The value packing is the only per-type part: the static floatBits/floatOf pair
 * is the bit-cast, wired into the template's VEX_TO_WORD / VEX_FROM_WORD blanks.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveFloat (reactive/reactive_float.c)
 * ============================================================================
 * the 32-bit float typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_float.h):
 * ----------------------------------------------------------------------------
 *   ReactiveFloat { Reactive base; } // embed-first; word = bit-cast float
 *
 * PRIVATE HELPERS: floatBits(value) / floatOf(word) — the bit-cast pair (static)
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveFloat_0(), ReactiveFloat_1(initial), _free
 * Public Setters: (.h) ReactiveFloat_set(self, value)
 * Public Getters: (.h) ReactiveFloat_get(self)
 * ============================================================================
 */

static uintptr_t floatBits(float value) {
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return (uintptr_t) bits;
}

static float floatOf(uintptr_t word) {
    uint32_t bits = (uint32_t) word;
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

#define VEX_NAME       ReactiveFloat
#define VEX_T          float
#define VEX_TYPE_ID    TYPE_REACTIVE_FLOAT
#define VEX_TO_WORD(v)   floatBits(v)
#define VEX_FROM_WORD(w) floatOf(w)
#define VEX_ZERO       0.0f
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
