// reactive/reactive_vec2.c — an observable 2D vector, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_vec2.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveVec2
 * ============================================================================
 * An observable Vec2, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE. The
 * engine's atomic word holds a Vec2* (a published vector); a rebind is a change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveVec2 (reactive/reactive_vec2.c)
 * ============================================================================
 * the observable Vec2 (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_vec2.h):
 * ----------------------------------------------------------------------------
 *   ReactiveVec2 { Reactive base; } // embed-first; word = Vec2*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveVec2_0(), ReactiveVec2_1(initial), _free
 * Public Setters: (.h) ReactiveVec2_set(self, value)
 * Public Getters: (.h) ReactiveVec2_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(Vec2);
