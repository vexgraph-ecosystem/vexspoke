#ifndef REACTIVE_REACTIVE_OBJECT_H
#define REACTIVE_REACTIVE_OBJECT_H

#include "lang/rect/rectangle.h"
#include "lang/vec2/vec2.h"
#include "lang/vec3/vec3.h"
#include "lang/vec4/vec4.h"
#include "primitive/int_double.h"
#include "primitive/long_double.h"
#include "primitive/long_float.h"
#include "reactive/generic.h"

// reactive/reactive_object.h — the standard OBJECT reactives in one file.
//
// ;;INTENTION("these are the same one-line TYPEDEF_REACTIVE stamp applied to the
// standard object types (Vec2/3/4, Rectangle, and the 16-byte pairs). Mechanical
// stamps, not hand-authored classes — the Single Class Per File Law is waived BY
// INTENT here, one file pair instead of seven. Per the Conflict Triage Law.")
//
// Each word holds a NAME* (a published object); a rebind is a change. Reactive(Vec4),
// Reactive(Rectangle), Reactive(IntDouble), ... resolve here.

TYPEDEF_REACTIVE(Vec2);
TYPEDEF_REACTIVE(Vec3);
TYPEDEF_REACTIVE(Vec4);
TYPEDEF_REACTIVE(Rectangle);
TYPEDEF_REACTIVE(IntDouble);
TYPEDEF_REACTIVE(LongFloat);
TYPEDEF_REACTIVE(LongDouble);

#endif
