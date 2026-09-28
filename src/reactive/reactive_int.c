// reactive/reactive_int.c — the Java-int (32-bit signed) typed reactive,
// STAMPED from reactive/reactive_tmpl.inc.

#include "reactive/reactive_int.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveInt
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: it embeds
 * the engine (reactive/reactive.h) and the engine's atomic word IS the 32-bit
 * signed value (sign-extended on read). One compare detects a change. Java
 * semantics: int. Arena-allocated (TYPE_REACTIVE_INT) or embedded.
 *
 * The value packing is the only per-type part: VEX_TO_WORD sign-extends through
 * uint32_t and VEX_FROM_WORD reverses it. The engine, its observers, its dirty
 * flag, and the owner-affine drain are shared and unchanged.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveInt (reactive/reactive_int.c)
 * ============================================================================
 * the 32-bit signed typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_int.h):
 * ----------------------------------------------------------------------------
 *   ReactiveInt { Reactive base; } // embed-first; word = int32
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveInt_0(), ReactiveInt_1(initial), _free
 * Public Setters: (.h) ReactiveInt_set(self, value)
 * Public Getters: (.h) ReactiveInt_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveInt
#define VEX_T          int32_t
#define VEX_TYPE_ID    TYPE_REACTIVE_INT
#define VEX_TO_WORD(v)   ((uintptr_t) (uint32_t) (v))
#define VEX_FROM_WORD(w) ((int32_t) (uint32_t) (w))
#define VEX_ZERO       ((int32_t) 0)
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
