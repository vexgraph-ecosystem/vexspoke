#ifndef REACTIVE_REACTIVE_STRING_H
#define REACTIVE_REACTIVE_STRING_H

#include <stdbool.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "reactive/reactive.h"

// reactive/reactive_string.h — the Java-String typed reactive.
//
// The word holds a pointer to a string block (a headed uint8_t*), so the word is
// the pointer — a rebind is a change; in-place content mutation behind the
// pointer is not observed (publish a fresh block to signal a change). Java
// semantics: an immutable String reference.

typedef struct ReactiveString {
    Reactive base;
} ReactiveString;

ReactiveString *ReactiveString_0(void);
ReactiveString *ReactiveString_1(const uint8_t *initial);
#define ReactiveString(...) CONSTRUCTOR_DISPATCH(ReactiveString, __VA_ARGS__)
void ReactiveString_free(ReactiveString *self);

void ReactiveString_set(ReactiveString *self, const uint8_t *value);
const uint8_t *ReactiveString_get(const ReactiveString *self);

#endif
