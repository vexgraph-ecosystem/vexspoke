#ifndef REACTIVE_REACTIVE_CHAR_H
#define REACTIVE_REACTIVE_CHAR_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_char.h — the char typed reactive, STAMPED from
// reactive/reactive_tmpl.h.
//
// A word-sized reactive: the engine's atomic word IS the character. Java
// semantics: 16-bit unsigned char.

#define VEX_NAME ReactiveChar
#define VEX_T    uint16_t
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveChar(...) CONSTRUCTOR_DISPATCH(ReactiveChar, __VA_ARGS__)

#endif
