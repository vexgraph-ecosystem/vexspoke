#ifndef REACTIVE_REACTIVE_SHORT_H
#define REACTIVE_REACTIVE_SHORT_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_short.h — the Java-short (16-bit signed) typed reactive,
// STAMPED from reactive/reactive_tmpl.h.
//
// A word-sized reactive: the engine's atomic word IS the 16-bit signed value
// (sign-extended on read). Java semantics: short.

#define VEX_NAME ReactiveShort
#define VEX_T    int16_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveShort(...) CONSTRUCTOR_DISPATCH(ReactiveShort, __VA_ARGS__)

#endif
