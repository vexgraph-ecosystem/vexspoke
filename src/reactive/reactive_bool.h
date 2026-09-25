#ifndef REACTIVE_REACTIVE_BOOL_H
#define REACTIVE_REACTIVE_BOOL_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/reactive.h"

// reactive/reactive_bool.h — the Java-boolean typed reactive.
//
// A word-sized reactive: the engine's atomic word IS the value, so a
// ReactiveBool is one Reactive (embed-first: a ReactiveBool* is a Reactive*).
// Pick this type instead of a plain bool to make a variable reactive — it wires
// itself (observers, dirty, drain) with no setup.

typedef struct ReactiveBool {
    Reactive base;
} ReactiveBool;

ReactiveBool *ReactiveBool_0(void);
ReactiveBool *ReactiveBool_1(bool initial);
#define ReactiveBool(...) CONSTRUCTOR_DISPATCH(ReactiveBool, __VA_ARGS__)
void ReactiveBool_free(ReactiveBool *self);

void ReactiveBool_set(ReactiveBool *self, bool value);
bool ReactiveBool_get(const ReactiveBool *self);

#endif
