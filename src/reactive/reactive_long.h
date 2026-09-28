#ifndef REACTIVE_REACTIVE_LONG_H
#define REACTIVE_REACTIVE_LONG_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_long.h — the Java-long (64-bit signed) typed reactive,
// STAMPED from reactive/reactive_tmpl.h.
//
// A word-sized reactive: the engine's atomic word IS the 64-bit signed value.
// Java semantics: long.

#define VEX_NAME ReactiveLong
#define VEX_T    int64_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveLong(...) CONSTRUCTOR_DISPATCH(ReactiveLong, __VA_ARGS__)

#endif
