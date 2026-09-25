#ifndef REACTIVE_REACTIVE_DOUBLE_H
#define REACTIVE_REACTIVE_DOUBLE_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/reactive.h"

// reactive/reactive_double.h — the Java-double (64-bit) typed reactive.
//
// A word-sized reactive: the double is BIT-CAST into the engine's word (never
// numerically converted), so every 64-bit pattern round-trips exactly. Java
// semantics: 64-bit IEEE-754.

typedef struct ReactiveDouble {
    Reactive base;
} ReactiveDouble;

ReactiveDouble *ReactiveDouble_0(void);
ReactiveDouble *ReactiveDouble_1(double initial);
#define ReactiveDouble(...) CONSTRUCTOR_DISPATCH(ReactiveDouble, __VA_ARGS__)
void ReactiveDouble_free(ReactiveDouble *self);

void ReactiveDouble_set(ReactiveDouble *self, double value);
double ReactiveDouble_get(const ReactiveDouble *self);

#endif
