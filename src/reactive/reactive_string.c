// reactive/reactive_string.c — the Java-String typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

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
 * A pointer-word typed reactive stamped from reactive/reactive_tmpl.inc: the
 * engine's atomic word holds a pointer to a string block. A rebind (a new
 * pointer) is a change; in-place content mutation behind the pointer is invisible
 * (publish a fresh block to signal). Java semantics: an immutable String
 * reference. Arena-allocated (TYPE_REACTIVE_STRING) or embedded.
 *
 * The value packing is the only per-type part: VEX_TO_WORD passes the pointer
 * through unchanged; VEX_FROM_WORD reads it back.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveString (reactive/reactive_string.c)
 * ============================================================================
 * the String typed reactive (an embedded Reactive engine; word = a block ptr),
 * stamped from reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_string.h):
 * ----------------------------------------------------------------------------
 *   ReactiveString { Reactive base; } // embed-first; word = uint8_t* block
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveString_0(), ReactiveString_1(initial), _free
 * Public Setters: (.h) ReactiveString_set(self, value)
 * Public Getters: (.h) ReactiveString_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveString
#define VEX_T          const uint8_t *
#define VEX_TYPE_ID    TYPE_REACTIVE_STRING
#define VEX_TO_WORD(v)   ((uintptr_t) (v))
#define VEX_FROM_WORD(w) ((const uint8_t*) (w))
#define VEX_ZERO       nullptr
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
