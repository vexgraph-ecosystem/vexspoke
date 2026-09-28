#ifndef REACTIVE_REACTIVE_BYTE_H
#define REACTIVE_REACTIVE_BYTE_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_byte.h — the Java-byte (8-bit signed) typed reactive, STAMPED
// from reactive/reactive_tmpl.h.
//
// A word-sized reactive: the engine's atomic word IS the 8-bit signed value
// (sign-extended on read). Java semantics: byte.

#define VEX_NAME ReactiveByte
#define VEX_T    int8_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveByte(...) CONSTRUCTOR_DISPATCH(ReactiveByte, __VA_ARGS__)

#endif
