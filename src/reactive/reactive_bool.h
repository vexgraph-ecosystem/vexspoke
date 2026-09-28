#ifndef REACTIVE_REACTIVE_BOOL_H
#define REACTIVE_REACTIVE_BOOL_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/generic.h"
#include "reactive/reactive.h"

// reactive/reactive_bool.h — the Java-boolean typed reactive, STAMPED from
// reactive/reactive_tmpl.h.
//
// A word-sized reactive: the engine's atomic word IS 0 or 1, so a ReactiveBool is
// one Reactive (embed-first: a ReactiveBool* is a Reactive*). Pick this type
// instead of a plain bool to make a variable reactive — it wires itself (observers,
// dirty, drain) with no setup. Java semantics: boolean.

#define VEX_NAME ReactiveBool
#define VEX_T    bool
#include "reactive/reactive_tmpl.h"
#undef VEX_NAME
#undef VEX_T

#define ReactiveBool(...) CONSTRUCTOR_DISPATCH(ReactiveBool, __VA_ARGS__)

#endif
