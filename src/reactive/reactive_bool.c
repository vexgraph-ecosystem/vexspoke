// reactive/reactive_bool.c — the Java-boolean typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_bool.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveBool
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: it embeds
 * the engine (reactive/reactive.h) and the engine's atomic word IS 0 or 1. One
 * compare detects a change; the engine supplies the observers, dirty flag, and
 * owner-affine drain. Java semantics: boolean. Arena-allocated
 * (TYPE_REACTIVE_BOOL) or embedded.
 *
 * The value packing is the only per-type part: VEX_TO_WORD maps true/false to
 * 1/0; VEX_FROM_WORD maps any nonzero word back to true.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveBool (reactive/reactive_bool.c)
 * ============================================================================
 * the boolean typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_bool.h):
 * ----------------------------------------------------------------------------
 *   ReactiveBool { Reactive base; } // embed-first; word = 0/1
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveBool_0(), ReactiveBool_1(initial), _free
 * Public Setters: (.h) ReactiveBool_set(self, value)
 * Public Getters: (.h) ReactiveBool_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveBool
#define VEX_T          bool
#define VEX_TYPE_ID    TYPE_REACTIVE_BOOL
#define VEX_TO_WORD(v)   ((uintptr_t) ((v) ? 1u : 0u))
#define VEX_FROM_WORD(w) ((w) != 0u)
#define VEX_ZERO       false
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
