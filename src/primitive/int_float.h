#ifndef PRIMITIVE_INT_FLOAT_H
#define PRIMITIVE_INT_FLOAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bit/bit.h"

#include "c23/constructor.h"

// primitive/int_float.h — IntFloat primitive (Legacy: primitive/IntFloat.java).
// Delegates to Bit64 width pool (8B stride).

// IntFloat — the value: a number as an integer scalar plus a normalized
// fractional part in [-1, 1). Keeping the magnitude in the integer and the
// remainder in a bounded fraction holds the value precise and consistent (the
// integer never loses low bits to float drift). Layout is 8 bytes — exactly one
// engine word, so a reactive over it rides the word by bit-cast.
typedef struct IntFloat {
    int32_t scalar;   // the integer part
    float   decimal;  // the fractional part, in [-1, 1)
} IntFloat;

_Static_assert(sizeof(IntFloat) == 8, "IntFloat must be one 8-byte word");

extern BitPool g_int_floatPool;

bool IntFloat_init(void);
void IntFloat_shutdown(void);
void *IntFloat_alloc(void);
void *IntFloat_allocArray(size_t count);
void IntFloat_free(void *ptr);
int64_t IntFloat_get(void *ptr);
void IntFloat_set(void *ptr, int64_t value);
bool IntFloat_compareAndSet(void *ptr, int64_t expected, int64_t value);
uint64_t IntFloat_type(void *ptr);
size_t IntFloat_length(void *ptr);


// Ergonomic constructors — IntFloat() and IntFloat(v1, v2)
void *IntFloat_allocWithValues(int32_t v1, float v2);
#define IntFloat_0(...) IntFloat_alloc()
#define IntFloat_2(v1, v2) IntFloat_allocWithValues(v1, v2)
#define IntFloat(...) CONSTRUCTOR_DISPATCH(IntFloat, __VA_ARGS__)


#define IntFloat_array(count) IntFloat_allocArray(count)

#endif
