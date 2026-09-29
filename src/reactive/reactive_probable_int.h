#ifndef REACTIVE_REACTIVE_PROBABLE_INT_H
#define REACTIVE_REACTIVE_PROBABLE_INT_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_probable_int.h — the int ReactiveProbable, STAMPED from
// reactive/reactive_probable_tmpl.h.
//
// The merged observable probable: { int32_t value, float chance } with chance in
// [0, 1]. get() yields the value with that probability, else 0. Spell it
// ReactiveProbable(Int) at call sites.

#define VEX_NAME ReactiveProbableInt
#define VEX_T    int32_t
#include "reactive/reactive_probable_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveProbableInt(...) CONSTRUCTOR_DISPATCH(ReactiveProbableInt, __VA_ARGS__)

#endif
