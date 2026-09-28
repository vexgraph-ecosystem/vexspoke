// reactive/reactive_vec4.c — an observable 4D vector, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_vec4.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveVec4
 * ============================================================================
 * An observable Vec4, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE. The
 * engine's atomic word holds a Vec4* (a published vector); a rebind is a change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveVec4 (reactive/reactive_vec4.c)
 * ============================================================================
 * the observable Vec4 (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_vec4.h):
 * ----------------------------------------------------------------------------
 *   ReactiveVec4 { Reactive base; } // embed-first; word = Vec4*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveVec4_0(), ReactiveVec4_1(initial), _free
 * Public Setters: (.h) ReactiveVec4_set(self, value)
 * Public Getters: (.h) ReactiveVec4_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(Vec4);
