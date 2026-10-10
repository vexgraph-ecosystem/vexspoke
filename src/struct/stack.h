#ifndef STRUCT_STACK_H
#define STRUCT_STACK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"

#include "struct/collection.h"
#include "exception/try_value.h"

// struct/stack.h — the Stack class, ported from struct/Stack.java. LIFO over a
// stride-based buffer that grows in chunks.

typedef struct Stack {
    Collection collection;
} Stack;

Stack *Stack_2(uint32_t element_class, size_t capacity);

// Stack pre-filled to count slots (legacy allocate): active_count = count.
Stack *Stack_2Count(uint32_t element_class, size_t count);

void Stack_free(Stack *stack);

// void for family consistency, but never silent: a growth whose arena
// allocation fails emits its THROW diagnostic and refuses the element.
// Callers that need the reason use Stack_pushTry.
void Stack_push(Stack *stack, uint64_t value_or_pointer);

// `;;CHECKER` half of the pair; Stack_push is the `;;HOTCODE` half. Returns the
// pushed value on TRY_OK, or the named reason: TRY_NULL_ARG, TRY_OVERFLOW
// (capacity would exceed uint32) or TRY_NO_MEMORY (arena could not grow). A
// rejected push leaves the stack unchanged and still THROWs.
TryValue Stack_pushTry(Stack *stack, uint64_t value_or_pointer);

uint64_t Stack_pop(Stack *stack);
uint64_t Stack_peek(Stack *stack);
uint8_t *Stack_slot(Stack *stack, size_t index);
bool Stack_isEmpty(Stack *stack);
size_t Stack_size(Stack *stack);
// Refused-grow count since construction (the Exhaustion Loudness Law).
;;TEST
uint64_t Stack_exhaustionCount(const Stack *stack);
size_t Stack_length(Stack *stack);
size_t Stack_capacity(Stack *stack);
uint32_t Stack_elementClassId(Stack *stack);
size_t Stack_stride(Stack *stack);
// Borrowed view of the backing buffer; used by tests to verify the block, not
// needed by production code.
;;TEST
uint8_t *Stack_dataBuffer(Stack *stack);


Stack *Stack_1(uint32_t element_class);

#define Stack(...) CONSTRUCTOR_DISPATCH(Stack, __VA_ARGS__)

#endif