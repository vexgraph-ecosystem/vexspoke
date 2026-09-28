// reactive/reactive_rectangle.c — an observable Rectangle, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_rectangle.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveRectangle
 * ============================================================================
 * An observable Rectangle, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE.
 * The engine's atomic word holds a Rectangle* (a published rect); a rebind is a
 * change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveRectangle (reactive/reactive_rectangle.c)
 * ============================================================================
 * the observable Rectangle (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_rectangle.h):
 * ----------------------------------------------------------------------------
 *   ReactiveRectangle { Reactive base; } // embed-first; word = Rectangle*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveRectangle_0(), ReactiveRectangle_1(initial), _free
 * Public Setters: (.h) ReactiveRectangle_set(self, value)
 * Public Getters: (.h) ReactiveRectangle_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(Rectangle);
