// reactive/reactive_byte.c — the Java-byte typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_byte.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveByte
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: it embeds
 * the engine and the engine's atomic word IS the 8-bit signed value
 * (sign-extended on read). Java semantics: byte. Arena-allocated
 * (TYPE_REACTIVE_BYTE) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveByte (reactive/reactive_byte.c)
 * ============================================================================
 * the 8-bit signed typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_byte.h):
 * ----------------------------------------------------------------------------
 *   ReactiveByte { Reactive base; } // embed-first; word = int8
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveByte_0(), ReactiveByte_1(initial), _free
 * Public Setters: (.h) ReactiveByte_set(self, value)
 * Public Getters: (.h) ReactiveByte_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveByte
#define VEX_T          int8_t
#define VEX_TYPE_ID    TYPE_REACTIVE_BYTE
#define VEX_TO_WORD(v)   ((uintptr_t) (uint8_t) (v))
#define VEX_FROM_WORD(w) ((int8_t) (uint8_t) (w))
#define VEX_ZERO       ((int8_t) 0)
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
