#ifndef REACTIVE_REACTIVE_DOUBLE_H
#define REACTIVE_REACTIVE_DOUBLE_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_double.h — the Java-double (64-bit) typed reactive, STAMPED
// from reactive/reactive_tmpl.h.
//
// A word-sized reactive: the double is BIT-CAST into the engine's word (never
// numerically converted), so every 64-bit pattern round-trips exactly. Java
// semantics: 64-bit IEEE-754.

#define VEX_NAME ReactiveDouble
#define VEX_T    double
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveDouble(...) CONSTRUCTOR_DISPATCH(ReactiveDouble, __VA_ARGS__)

#endif
