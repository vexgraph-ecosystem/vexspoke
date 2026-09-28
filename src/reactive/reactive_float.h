#ifndef REACTIVE_REACTIVE_FLOAT_H
#define REACTIVE_REACTIVE_FLOAT_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_float.h — the Java-float (32-bit) typed reactive, STAMPED
// from reactive/reactive_tmpl.h.
//
// A word-sized reactive: the float is BIT-CAST into the engine's word (never
// numerically converted), so every 32-bit pattern round-trips exactly. Java
// semantics: 32-bit IEEE-754.

#define VEX_NAME ReactiveFloat
#define VEX_T    float
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveFloat(...) CONSTRUCTOR_DISPATCH(ReactiveFloat, __VA_ARGS__)

#endif
