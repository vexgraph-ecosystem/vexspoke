#ifndef STRUCT_ARRAY_H
#define STRUCT_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"

#include "struct/collection.h"
#include "exception/try_value.h"

// struct/array.h — the Array class, ported from struct/Array.java.
//
// Fixed-length, zero-initialized, stride-based array. Active count equals
// capacity; unlike List it never grows.

typedef struct Array {
    Collection collection;
} Array;

// Fixed array of length elements of element_class, all zero-initialized.
Array *Array_2(uint32_t element_class, size_t length);

// Free the array and its data buffer.
void Array_free(Array *array);

// Get/set the value or pointer at index (bounds-checked).
uint64_t Array_get(Array *array, size_t index);
void Array_set(Array *array, size_t index, uint64_t value);

// Cold, checking accessor: the value or the named reason it is absent — the
// `;;CHECKER` half of the pair; Array_get is the `;;HOTCODE` half.
TryValue Array_getTry(Array *array, size_t index);

// Pointer to the struct element at index (bounds-checked).
uint8_t *Array_slot(Array *array, size_t index);

bool Array_isEmpty(Array *array);
size_t Array_size(Array *array);
size_t Array_length(Array *array);
size_t Array_capacity(Array *array);
uint32_t Array_elementClassId(Array *array);
size_t Array_stride(Array *array);
uint8_t *Array_dataBuffer(Array *array);


#define Array(...) CONSTRUCTOR_DISPATCH(Array, __VA_ARGS__)

#endif