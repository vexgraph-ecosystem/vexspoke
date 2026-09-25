#ifndef REACTIVE_REACTIVE_INT_H
#define REACTIVE_REACTIVE_INT_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/reactive.h"

// reactive/reactive_int.h — the Java-int (32-bit signed) typed reactive.
//
// A word-sized reactive: the engine's atomic word IS the value, so a ReactiveInt
// is one Reactive (embed-first: a ReactiveInt* is a Reactive*). Java semantics:
// 32-bit signed.

typedef struct ReactiveInt {
    Reactive base;
} ReactiveInt;

ReactiveInt *ReactiveInt_0(void);
ReactiveInt *ReactiveInt_1(int32_t initial);
#define ReactiveInt(...) CONSTRUCTOR_DISPATCH(ReactiveInt, __VA_ARGS__)
void ReactiveInt_free(ReactiveInt *self);

void ReactiveInt_set(ReactiveInt *self, int32_t value);
int32_t ReactiveInt_get(const ReactiveInt *self);

#endif
