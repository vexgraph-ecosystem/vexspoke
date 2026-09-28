// reactive/reactive_short.c — the Java-short typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_short.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveShort
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: it embeds
 * the engine and the engine's atomic word IS the 16-bit signed value
 * (sign-extended on read). Java semantics: short. Arena-allocated
 * (TYPE_REACTIVE_SHORT) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveShort (reactive/reactive_short.c)
 * ============================================================================
 * the 16-bit signed typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_short.h):
 * ----------------------------------------------------------------------------
 *   ReactiveShort { Reactive base; } // embed-first; word = int16
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveShort_0(), ReactiveShort_1(initial), _free
 * Public Setters: (.h) ReactiveShort_set(self, value)
 * Public Getters: (.h) ReactiveShort_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveShort
#define VEX_T          int16_t
#define VEX_TYPE_ID    TYPE_REACTIVE_SHORT
#define VEX_TO_WORD(v)   ((uintptr_t) (uint16_t) (v))
#define VEX_FROM_WORD(w) ((int16_t) (uint16_t) (w))
#define VEX_ZERO       ((int16_t) 0)
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
