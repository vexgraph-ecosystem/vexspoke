// reactive/reactive_long.c — the Java-long typed reactive, STAMPED from
// reactive/reactive_tmpl.inc.

#include "reactive/reactive_long.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveLong
 * ============================================================================
 * A word-sized typed reactive stamped from reactive/reactive_tmpl.inc: it embeds
 * the engine and the engine's atomic word IS the 64-bit signed value. Java
 * semantics: long. Arena-allocated (TYPE_REACTIVE_LONG) or embedded.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveLong (reactive/reactive_long.c)
 * ============================================================================
 * the 64-bit signed typed reactive (an embedded Reactive engine), stamped from
 * reactive/reactive_tmpl.inc.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_long.h):
 * ----------------------------------------------------------------------------
 *   ReactiveLong { Reactive base; } // embed-first; word = int64
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveLong_0(), ReactiveLong_1(initial), _free
 * Public Setters: (.h) ReactiveLong_set(self, value)
 * Public Getters: (.h) ReactiveLong_get(self)
 * ============================================================================
 */

#define VEX_NAME       ReactiveLong
#define VEX_T          int64_t
#define VEX_TYPE_ID    TYPE_REACTIVE_LONG
#define VEX_TO_WORD(v)   ((uintptr_t) (v))
#define VEX_FROM_WORD(w) ((int64_t) (w))
#define VEX_ZERO       ((int64_t) 0)
#include "reactive/reactive_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
