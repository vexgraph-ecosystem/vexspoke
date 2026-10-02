// reactive/reactive_probable.c — the ReactiveProbable family in one file.
// See reactive/reactive_probable.h (the ;;INTENTION).

#include "reactive/reactive_probable.h"

#include <stdio.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "util/random.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveProbable (the probable family)
 * ============================================================================
 * The observable probable (one template, instantiated per value type). { value,
 * chance } with chance in [0, 1]: get() rolls — with probability `chance` it
 * yields the value, else the empty value. _set publishes a new value+chance
 * (atomic, never fires; the owner drains). Carries the four typed channels.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: ReactiveProbable (reactive/reactive_probable.c)
 * ============================================================================
 * the probable-reactive family (template instantiations) — currently Int.
 *
 * STRUCT FIELDS: none — the classes live in the header; this file holds the bodies
 * (the shared random-based rolling get + the channel trampolines).
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Per class (emitted by the template): _0/_2, _free, _set, _get, _getChance, and
 * the four typed channels.
 * ============================================================================
 */

#define VEX_NAME       ReactiveProbableInt
#define VEX_T          int32_t
#define VEX_TYPE_ID    TYPE_REACTIVE_PROBABLE
#define VEX_TO_WORD(v)   ((uintptr_t) (uint32_t) (v))
#define VEX_FROM_WORD(w) ((int32_t) (uint32_t) (w))
#define VEX_ZERO       ((int32_t) 0)
#define VEX_VALUE_OF(out, cap, v) snprintf((out), (cap), "%d", (int) (v))
#include "reactive/reactive_probable_tmpl.inc"
#undef VEX_NAME
#undef VEX_T
#undef VEX_TYPE_ID
#undef VEX_TO_WORD
#undef VEX_FROM_WORD
#undef VEX_ZERO
#undef VEX_VALUE_OF
