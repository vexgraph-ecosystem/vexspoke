// reactive/reactive_object.c — the standard object reactives in one file.
// See reactive/reactive_object.h (the ;;INTENTION for the Single Class Per File
// waiver).

#include "reactive/reactive_object.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: ReactiveObject (the standard object family)
 * ============================================================================
 * The standard object typed reactives (Vec2, Vec3, Vec4, Rectangle, IntDouble,
 * LongFloat, LongDouble), each stamped by TYPEDEF_REACTIVE / IMPLEMENT_REACTIVE.
 * The word holds the object pointer; a rebind is a change. They share this file
 * by intent (see the header).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: ReactiveObject (reactive/reactive_object.c)
 * ============================================================================
 * the standard object-reactive family (seven TYPEDEF_REACTIVE stamps).
 *
 * STRUCT FIELDS: none — the classes live in the header; this file holds the bodies.
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Per class (emitted by the macro): _0/_1, _free, _set, _get, and the four typed
 * channels (addOn/removeOn Set/Changed/Get/Nullptr).
 * ============================================================================
 */

IMPLEMENT_REACTIVE(Vec2);
IMPLEMENT_REACTIVE(Vec3);
IMPLEMENT_REACTIVE(Vec4);
IMPLEMENT_REACTIVE(Rectangle);
IMPLEMENT_REACTIVE(IntDouble);
IMPLEMENT_REACTIVE(LongFloat);
IMPLEMENT_REACTIVE(LongDouble);
