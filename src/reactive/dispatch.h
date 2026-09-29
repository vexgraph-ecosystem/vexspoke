#ifndef REACTIVE_DISPATCH_H
#define REACTIVE_DISPATCH_H

#include "reactive/generic.h"
#include "reactive/reactive.h"
#include "reactive/reactive_bool.h"
#include "reactive/reactive_byte.h"
#include "reactive/reactive_short.h"
#include "reactive/reactive_char.h"
#include "reactive/reactive_int.h"
#include "reactive/reactive_long.h"
#include "reactive/reactive_float.h"
#include "reactive/reactive_double.h"
#include "reactive/reactive_string.h"
#include "reactive/reactive_vec2.h"
#include "reactive/reactive_vec3.h"
#include "reactive/reactive_vec4.h"
#include "reactive/reactive_rectangle.h"
#include "reactive/reactive_int_float.h"
#include "reactive/reactive_int_double.h"
#include "reactive/reactive_long_float.h"
#include "reactive/reactive_long_double.h"

// reactive/dispatch.h — the generic channel surface (the Java-like face).
//
// Reactive_addOnChanged(r, fn) picks the right per-class arm from r's type at
// compile time, so the caller writes ONE name across the whole family and never
// the class. A bare Reactive* routes to the raw engine channel. The macros are
// variadic so a raw arm may carry its userdata while the typed arms (fn(T value))
// do not.
//
//     Reactive(Int) *gold = Reactive(Int)(100);
//     Reactive_addOnChanged(gold, onGoldChanged);   // -> ReactiveInt_addOnChanged
//
// The typed callback is fn(T value): the VALUE is the payload, so the reactive
// identity is not passed. Remove matches the function-pointer address.

#define VEX_REACTIVE_FAMILY(X) \
    X(Bool) X(Byte) X(Short) X(Char) X(Int) X(Long) X(Float) X(Double) X(String) \
    X(Vec2) X(Vec3) X(Vec4) X(Rectangle) \
    X(IntFloat) X(IntDouble) X(LongFloat) X(LongDouble)

#define VEX_CHAN_ARM(T, SUF) VEX_CAT(Reactive, T) *: VEX_CAT(VEX_CAT(Reactive, T), SUF),

#define VEX_ARM_ADD_SET(T)       VEX_CHAN_ARM(T, _addOnSet)
#define VEX_ARM_REM_SET(T)       VEX_CHAN_ARM(T, _removeOnSet)
#define VEX_ARM_ADD_CHANGED(T)   VEX_CHAN_ARM(T, _addOnChanged)
#define VEX_ARM_REM_CHANGED(T)   VEX_CHAN_ARM(T, _removeOnChanged)
#define VEX_ARM_ADD_GET(T)       VEX_CHAN_ARM(T, _addOnGet)
#define VEX_ARM_REM_GET(T)       VEX_CHAN_ARM(T, _removeOnGet)
#define VEX_ARM_ADD_NULLPTR(T)   VEX_CHAN_ARM(T, _addOnNullptr)
#define VEX_ARM_REM_NULLPTR(T)   VEX_CHAN_ARM(T, _removeOnNullptr)

#define Reactive_addOnSet(r, ...)        _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_ADD_SET)     default: Reactive_watchSet)((r), __VA_ARGS__)
#define Reactive_removeOnSet(r, ...)     _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_REM_SET)     default: Reactive_unwatchSet)((r), __VA_ARGS__)
#define Reactive_addOnChanged(r, ...)    _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_ADD_CHANGED) default: Reactive_watchChanged)((r), __VA_ARGS__)
#define Reactive_removeOnChanged(r, ...) _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_REM_CHANGED) default: Reactive_unwatchChanged)((r), __VA_ARGS__)
#define Reactive_addOnGet(r, ...)        _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_ADD_GET)     default: Reactive_watchGet)((r), __VA_ARGS__)
#define Reactive_removeOnGet(r, ...)     _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_REM_GET)     default: Reactive_unwatchGet)((r), __VA_ARGS__)
#define Reactive_addOnNullptr(r, ...)    _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_ADD_NULLPTR) default: Reactive_watchNullptr)((r), __VA_ARGS__)
#define Reactive_removeOnNullptr(r, ...) _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_REM_NULLPTR) default: Reactive_unwatchNullptr)((r), __VA_ARGS__)

// --- the generic value accessors (a reactive reads like a variable) ---
// Reactive_set(gold, 100) / Reactive_get(gold) resolve to the per-class _set/_get
// by the reactive's type; a bare Reactive* (or nullptr) routes to the raw engine.
#define VEX_ARM_VAL_SET(T) VEX_CAT(Reactive, T) *: VEX_CAT(VEX_CAT(Reactive, T), _set),
#define VEX_ARM_VAL_GET(T) VEX_CAT(Reactive, T) *: VEX_CAT(VEX_CAT(Reactive, T), _get),

#define Reactive_set(r, ...) _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_VAL_SET) default: Reactive_store)((r), __VA_ARGS__)
#define Reactive_get(r)      _Generic((r), VEX_REACTIVE_FAMILY(VEX_ARM_VAL_GET) default: Reactive_load)((r))

#endif
