// reactive/reactive_primitive.c — the whole scalar reactive family in one file.
// See reactive/reactive_primitive.h (the ;;INTENTION for the Single Class Per File
// waiver).

#include "reactive/reactive_primitive.h"

#include <stdio.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactivePrimitive (the scalar family)
 * ============================================================================
 * The ten scalar typed reactives (Bool, Byte, Short, Char, Int, Long, Float,
 * Double, String, IntFloat) stamped from reactive/reactive_tmpl.inc. Each embeds
 * the one engine and differs ONLY in how it packs its value into the word: an
 * integer cast, a bit-cast (float/double/IntFloat), or a pointer (String). They
 * share this file by intent (see the header), since they are mechanical stamps.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: ReactivePrimitive (reactive/reactive_primitive.c)
 * ============================================================================
 * the scalar typed-reactive family (ten template instantiations) + the bit-cast
 * helpers.
 *
 * STRUCT FIELDS: none — the classes live in the header; this file holds the
 * bodies and the static bit-cast pairs (floatBits/floatOf, bitsOf/doubleOf,
 * intFloatBits/intFloatOf/intFloatZero).
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Per class (emitted by the template): _0/_1, _free, _set, _get, and the four
 * typed channels (addOn/removeOn Set/Changed/Get/Nullptr).
 * ============================================================================
 */

// --- bit-cast helpers (shared by the float/double/IntFloat packings) ---

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

// --- the ten instantiations ---

#define VEX_NAME ReactiveBool
#define VEX_T    bool
#define VEX_TYPE_ID TYPE_REACTIVE_BOOL
#define VEX_TO_WORD(v)   ((uintptr_t) ((v) ? 1u : 0u))
#define VEX_FROM_WORD(w) ((w) != 0u)
#define VEX_ZERO       false
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%s", (v) ? "true" : "false")
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveByte
#define VEX_T    int8_t
#define VEX_TYPE_ID TYPE_REACTIVE_BYTE
#define VEX_TO_WORD(v)   ((uintptr_t) (uint8_t) (v))
#define VEX_FROM_WORD(w) ((int8_t) (uint8_t) (w))
#define VEX_ZERO       ((int8_t) 0)
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%d", (int) (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveShort
#define VEX_T    int16_t
#define VEX_TYPE_ID TYPE_REACTIVE_SHORT
#define VEX_TO_WORD(v)   ((uintptr_t) (uint16_t) (v))
#define VEX_FROM_WORD(w) ((int16_t) (uint16_t) (w))
#define VEX_ZERO       ((int16_t) 0)
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%d", (int) (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveChar
#define VEX_T    uint16_t
#define VEX_TYPE_ID TYPE_REACTIVE_CHAR
#define VEX_TO_WORD(v)   ((uintptr_t) (uint16_t) (v))
#define VEX_FROM_WORD(w) ((uint16_t) (w))
#define VEX_ZERO       ((uint16_t) 0)
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%u", (unsigned) (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveInt
#define VEX_T    int32_t
#define VEX_TYPE_ID TYPE_REACTIVE_INT
#define VEX_TO_WORD(v)   ((uintptr_t) (uint32_t) (v))
#define VEX_FROM_WORD(w) ((int32_t) (uint32_t) (w))
#define VEX_ZERO       ((int32_t) 0)
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%d", (int) (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveLong
#define VEX_T    int64_t
#define VEX_TYPE_ID TYPE_REACTIVE_LONG
#define VEX_TO_WORD(v)   ((uintptr_t) (v))
#define VEX_FROM_WORD(w) ((int64_t) (w))
#define VEX_ZERO       ((int64_t) 0)
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%lld", (long long) (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveFloat
#define VEX_T    float
#define VEX_TYPE_ID TYPE_REACTIVE_FLOAT
#define VEX_TO_WORD(v)   floatBits(v)
#define VEX_FROM_WORD(w) floatOf(w)
#define VEX_ZERO       0.0f
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%g", (double) (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveDouble
#define VEX_T    double
#define VEX_TYPE_ID TYPE_REACTIVE_DOUBLE
#define VEX_TO_WORD(v)   bitsOf(v)
#define VEX_FROM_WORD(w) doubleOf(w)
#define VEX_ZERO       0.0
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%g", (v))
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveString
#define VEX_T    const uint8_t *
#define VEX_TYPE_ID TYPE_REACTIVE_STRING
#define VEX_TO_WORD(v)   ((uintptr_t) (v))
#define VEX_FROM_WORD(w) ((const uint8_t*) (w))
#define VEX_ZERO       nullptr
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%s", (v) ? (const char*) (v) : "")
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF

#define VEX_NAME ReactiveIntFloat
#define VEX_T    IntFloat
#define VEX_TYPE_ID TYPE_REACTIVE_INT_FLOAT
#define VEX_TO_WORD(v)   intFloatBits(v)
#define VEX_FROM_WORD(w) intFloatOf(w)
#define VEX_ZERO       intFloatZero()
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%g", (double) (v).scalar + (double) (v).decimal)
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF
