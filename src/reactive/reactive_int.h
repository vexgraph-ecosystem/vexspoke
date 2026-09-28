#ifndef REACTIVE_REACTIVE_INT_H
#define REACTIVE_REACTIVE_INT_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_int.h — the Java-int (32-bit signed) typed reactive,
// STAMPED from reactive/reactive_tmpl.h.
//
// A word-sized reactive: the engine's atomic word IS the value, so a ReactiveInt
// is one Reactive (embed-first: a ReactiveInt* is a Reactive*). Java semantics:
// 32-bit signed. Spell it Reactive(Int) at call sites.

#define VEX_NAME ReactiveInt
#define VEX_T    int32_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveInt(...) CONSTRUCTOR_DISPATCH(ReactiveInt, __VA_ARGS__)

#endif
