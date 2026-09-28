#ifndef REACTIVE_REACTIVE_INT_FLOAT_H
#define REACTIVE_REACTIVE_INT_FLOAT_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "primitive/int_float.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_int_float.h — the IntFloat typed reactive, STAMPED from
// reactive/reactive_tmpl.h.
//
// A word-sized reactive: an IntFloat (an integer scalar + a fraction in [-1, 1))
// is 8 bytes, so the engine's atomic word IS the value (bit-cast — the exact
// 8 bytes round-trip). One compare detects a change. Spell it Reactive(IntFloat).

#define VEX_NAME ReactiveIntFloat
#define VEX_T    IntFloat
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveIntFloat(...) CONSTRUCTOR_DISPATCH(ReactiveIntFloat, __VA_ARGS__)

#endif
