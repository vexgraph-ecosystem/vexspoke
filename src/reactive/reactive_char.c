// reactive/reactive_char.c — the char typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_char.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveChar
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: it embeds
 * the engine and the engine's atomic word IS the character. C `char` semantics.
 * Arena-allocated (TYPE_REACTIVE_CHAR) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveChar (reactive/reactive_char.c)
 * ============================================================================
 * the char typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_char.h):
 * ----------------------------------------------------------------------------
 *   ReactiveChar { Reactive base; } // embed-first; word = char
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveChar_0(), ReactiveChar_1(initial), _free
 * Public Setters: (.h) ReactiveChar_set(self, value)
 * Public Getters: (.h) ReactiveChar_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveChar
#define VEX_T          uint16_t
#define VEX_TYPE_ID    TYPE_REACTIVE_CHAR
#define VEX_TO_WORD(v)   ((uintptr_t) (uint16_t) (v))
#define VEX_FROM_WORD(w) ((uint16_t) (w))
#define VEX_ZERO       ((uint16_t) 0)
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
