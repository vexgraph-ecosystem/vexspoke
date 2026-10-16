#ifndef REACTIVE_REACTIVE_OBJECT_H
#define REACTIVE_REACTIVE_OBJECT_H

#include "c23/constructor.h"
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

// TYPEDEF_REACTIVE stamps the constructor arms; expose normal class chooser
// macros for the standard object family at the consumer boundary.
#define ReactiveVec2(...) CONSTRUCTOR_DISPATCH(ReactiveVec2, __VA_ARGS__)
#define ReactiveVec3(...) CONSTRUCTOR_DISPATCH(ReactiveVec3, __VA_ARGS__)
#define ReactiveVec4(...) CONSTRUCTOR_DISPATCH(ReactiveVec4, __VA_ARGS__)
#define ReactiveRectangle(...) CONSTRUCTOR_DISPATCH(ReactiveRectangle, __VA_ARGS__)
#define ReactiveIntDouble(...) CONSTRUCTOR_DISPATCH(ReactiveIntDouble, __VA_ARGS__)
#define ReactiveLongFloat(...) CONSTRUCTOR_DISPATCH(ReactiveLongFloat, __VA_ARGS__)
#define ReactiveLongDouble(...) CONSTRUCTOR_DISPATCH(ReactiveLongDouble, __VA_ARGS__)

#endif
