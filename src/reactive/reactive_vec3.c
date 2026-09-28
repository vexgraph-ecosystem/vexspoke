// reactive/reactive_vec3.c — an observable 3D vector, STAMPED by IMPLEMENT_REACTIVE.

#include "reactive/reactive_vec3.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveVec3
 * ============================================================================
 * An observable Vec3, stamped from TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE. The
 * engine's atomic word holds a Vec3* (a published vector); a rebind is a change.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ReactiveVec3 (reactive/reactive_vec3.c)
 * ============================================================================
 * the observable Vec3 (an embedded Reactive engine), stamped by the macro.
 *
 * STRUCT FIELDS (Mirroring reactive/reactive_vec3.h):
 * ----------------------------------------------------------------------------
 *   ReactiveVec3 { Reactive base; } // embed-first; word = Vec3*
 *
 * FUNCTION REGISTRY (emitted by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE):
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h) ReactiveVec3_0(), ReactiveVec3_1(initial), _free
 * Public Setters: (.h) ReactiveVec3_set(self, value)
 * Public Getters: (.h) ReactiveVec3_get(self)
 * ============================================================================
 */

IMPLEMENT_REACTIVE(Vec3);
