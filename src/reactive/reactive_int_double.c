// reactive/reactive_int_double.c — an observable IntDouble, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_int_double.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveIntDouble
 * ============================================================================
 * An observable IntDouble, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE.
 * The engine's atomic word holds an IntDouble* (a published pair); a rebind is a
 * change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveIntDouble (reactive/reactive_int_double.c)
 * ============================================================================
 * the observable IntDouble (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_int_double.h):
 * ----------------------------------------------------------------------------
 *   ReactiveIntDouble { Reactive base; } // embed-first; word = IntDouble*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveIntDouble_0(), ReactiveIntDouble_1(initial), _free
 * Public Setters: (.h) ReactiveIntDouble_set(self, value)
 * Public Getters: (.h) ReactiveIntDouble_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(IntDouble);
