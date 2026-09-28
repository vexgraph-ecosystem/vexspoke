#ifndef PRIMITIVE_INT_DOUBLE_H
#define PRIMITIVE_INT_DOUBLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bit/bit.h"

#include "c23/constructor.h"

// primitive/int_double.h — IntDouble primitive (Legacy: primitive/IntDouble.java).
// Delegates to Bit128 width pool (16B stride).

// IntDouble — the value: an int32 scalar plus a normalized double fraction in
// [-1, 1). 16 bytes (int32 + aligned double) — too big for one engine word.
typedef struct IntDouble {
    int32_t scalar;   // the integer part
    double  decimal;  // the fractional part, in [-1, 1)
} IntDouble;

_Static_assert(sizeof(IntDouble) == 16, "IntDouble must be 16 bytes");

extern BitPool g_int_doublePool;

bool IntDouble_init(void);
void IntDouble_shutdown(void);
void *IntDouble_alloc(void);
void *IntDouble_allocArray(size_t count);
void IntDouble_free(void *ptr);
int64_t IntDouble_get(void *ptr);
void IntDouble_set(void *ptr, int64_t value);
bool IntDouble_compareAndSet(void *ptr, int64_t expected, int64_t value);
uint64_t IntDouble_type(void *ptr);
size_t IntDouble_length(void *ptr);


// Ergonomic constructors — IntDouble() and IntDouble(v1, v2)
void *IntDouble_allocWithValues(int32_t v1, double v2);
#define IntDouble_0(...) IntDouble_alloc()
#define IntDouble_2(v1, v2) IntDouble_allocWithValues(v1, v2)
#define IntDouble(...) CONSTRUCTOR_DISPATCH(IntDouble, __VA_ARGS__)


#define IntDouble_array(count) IntDouble_allocArray(count)

#endif
