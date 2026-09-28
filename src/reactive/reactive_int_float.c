// reactive/reactive_int_float.c — the IntFloat typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_int_float.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveIntFloat
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: an
 * IntFloat (integer scalar + a normalized fraction in [-1, 1)) is 8 bytes, so
 * the engine's atomic word IS the value — bit-cast, never numerically converted,
 * so every bit pattern round-trips exactly. One compare detects a change.
 * Arena-allocated (TYPE_REACTIVE_INT_FLOAT) or embedded.
 *
 * The value packing is the only per-type part: the static intFloatBits/intFloatOf
 * pair is the bit-cast, wired into the template's VEX_TO_WORD / VEX_FROM_WORD.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveIntFloat (reactive/reactive_int_float.c)
 * ============================================================================
 * the IntFloat typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_int_float.h):
 * ----------------------------------------------------------------------------
 *   ReactiveIntFloat { Reactive base; } // embed-first; word = bit-cast IntFloat
 *
 * PRIVATE HELPERS: intFloatBits(v) / intFloatOf(w) / intFloatZero() (static)
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveIntFloat_0(), ReactiveIntFloat_1(initial), _free
 * Public Setters: (.h) ReactiveIntFloat_set(self, value)
 * Public Getters: (.h) ReactiveIntFloat_get(self)
 * ============================================================================
 */

static uintptr_t intFloatBits(IntFloat value) {
    uint64_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return (uintptr_t) bits;
}

static IntFloat intFloatOf(uintptr_t word) {
    uint64_t bits = (uint64_t) word;
    IntFloat value = { 0, 0.0f };
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static IntFloat intFloatZero(void) {
    IntFloat zero = { 0, 0.0f };
    return zero;
}

#define VEX_NAME       ReactiveIntFloat
#define VEX_T          IntFloat
#define VEX_TYPE_ID    TYPE_REACTIVE_INT_FLOAT
#define VEX_TO_WORD(v)   intFloatBits(v)
#define VEX_FROM_WORD(w) intFloatOf(w)
#define VEX_ZERO       intFloatZero()
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
