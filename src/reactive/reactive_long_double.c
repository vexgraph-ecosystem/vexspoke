// reactive/reactive_long_double.c — an observable LongDouble, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_long_double.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveLongDouble
 * ============================================================================
 * An observable LongDouble, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE.
 * The engine's atomic word holds a LongDouble* (a published pair); a rebind is a
 * change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveLongDouble (reactive/reactive_long_double.c)
 * ============================================================================
 * the observable LongDouble (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_long_double.h):
 * ----------------------------------------------------------------------------
 *   ReactiveLongDouble { Reactive base; } // embed-first; word = LongDouble*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveLongDouble_0(), ReactiveLongDouble_1(initial), _free
 * Public Setters: (.h) ReactiveLongDouble_set(self, value)
 * Public Getters: (.h) ReactiveLongDouble_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(LongDouble);
