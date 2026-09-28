#ifndef REACTIVE_REACTIVE_STRING_H
#define REACTIVE_REACTIVE_STRING_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_string.h — the Java-String typed reactive, STAMPED from
// reactive/reactive_tmpl.h.
//
// The word holds a pointer to a string block (a headed uint8_t*), so the word is
// the pointer — a rebind is a change; in-place content mutation behind the
// pointer is not observed (publish a fresh block to signal a change). Java
// semantics: an immutable String reference.

#define VEX_NAME ReactiveString
#define VEX_T    const uint8_t *
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveString(...) CONSTRUCTOR_DISPATCH(ReactiveString, __VA_ARGS__)

#endif
