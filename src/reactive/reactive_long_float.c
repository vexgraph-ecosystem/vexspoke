// reactive/reactive_long_float.c — an observable LongFloat, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_long_float.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveLongFloat
 * ============================================================================
 * An observable LongFloat, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE.
 * The engine's atomic word holds a LongFloat* (a published pair); a rebind is a
 * change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveLongFloat (reactive/reactive_long_float.c)
 * ============================================================================
 * the observable LongFloat (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_long_float.h):
 * ----------------------------------------------------------------------------
 *   ReactiveLongFloat { Reactive base; } // embed-first; word = LongFloat*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveLongFloat_0(), ReactiveLongFloat_1(initial), _free
 * Public Setters: (.h) ReactiveLongFloat_set(self, value)
 * Public Getters: (.h) ReactiveLongFloat_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(LongFloat);
