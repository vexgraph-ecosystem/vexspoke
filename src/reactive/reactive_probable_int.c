// reactive/reactive_probable_int.c — the int ReactiveProbable, STAMPED from
// reactive/reactive_probable_tmpl.inc.

#include "reactive/reactive_probable_int.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "util/random.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveProbableInt
 * ============================================================================
 * A merged observable probable (ReactiveProbable(Int)) stamped from
 * reactive/reactive_probable_tmpl.inc: { int32_t value, float chance }. get()
 * rolls — with probability `chance` it yields the value, else 0. _set publishes
 * a new value+chance (atomic, never fires; the owner drains, the engine
 * contract). Contains the four typed channels (onSet / onChanged / onGet /
 * onNullptr).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveProbableInt (reactive/reactive_probable_int.c)
 * ============================================================================
 * the int observable probable (an embedded Reactive engine), stamped from the
 * probable template.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_probable_int.h):
 * ----------------------------------------------------------------------------
 *   ReactiveProbableInt { Reactive base; float chance; } // word = int32
 *
 * FUNCTION REGISTRY (emitted by the template):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveProbableInt_0(), _2(value, chance), _free
 * Public Setters: (.h) ReactiveProbableInt_set(self, value, chance)
 * Public Getters: (.h) ReactiveProbableInt_get(self), _getChance(self)
 * Public Channels: (.h) the four typed +-trampoline channel pairs
 * ============================================================================
 */

#define VEX_NAME       ReactiveProbableInt
#define VEX_T          int32_t
#define VEX_TYPE_ID    TYPE_REACTIVE_PROBABLE
#define VEX_TO_WORD(v)   ((uintptr_t) (uint32_t) (v))
#define VEX_FROM_WORD(w) ((int32_t) (uint32_t) (w))
#define VEX_ZERO       ((int32_t) 0)
#include "reactive/reactive_probable_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
