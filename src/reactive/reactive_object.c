// reactive/reactive_object.c — the standard object reactives in one file.
// See reactive/reactive_object.h (the ;;INTENTION for the Single Class Per File
// waiver).

#include "reactive/reactive_object.h"

#include <stdio.h>

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

// The standard objects' valueOf(): render the FIELDS, not the pointer. A null
// value (the word is the object pointer) prints "null".
static void vec2_str(const Vec2 *v, char *out, size_t cap) {
    if (v == nullptr) { snprintf(out, cap, "null"); return; }
    snprintf(out, cap, "(%.3g, %.3g)", (double) v->x, (double) v->y);
}

static void vec3_str(const Vec3 *v, char *out, size_t cap) {
    if (v == nullptr) { snprintf(out, cap, "null"); return; }
    snprintf(out, cap, "(%.3g, %.3g, %.3g)", (double) v->x, (double) v->y, (double) v->z);
}

static void vec4_str(const Vec4 *v, char *out, size_t cap) {
    if (v == nullptr) { snprintf(out, cap, "null"); return; }
    snprintf(out, cap, "(%.3g, %.3g, %.3g, %.3g)",
             (double) v->x, (double) v->y, (double) v->z, (double) v->w);
}

static void rectangle_str(const Rectangle *r, char *out, size_t cap) {
    if (r == nullptr) { snprintf(out, cap, "null"); return; }
    snprintf(out, cap, "[%.3g, %.3g, %.3g x %.3g]",
             (double) r->x, (double) r->y, (double) r->width, (double) r->height);
}

IMPLEMENT_REACTIVE_STR(Vec2, vec2_str);
IMPLEMENT_REACTIVE_STR(Vec3, vec3_str);
IMPLEMENT_REACTIVE_STR(Vec4, vec4_str);
IMPLEMENT_REACTIVE_STR(Rectangle, rectangle_str);
IMPLEMENT_REACTIVE(IntDouble);
IMPLEMENT_REACTIVE(LongFloat);
IMPLEMENT_REACTIVE(LongDouble);
