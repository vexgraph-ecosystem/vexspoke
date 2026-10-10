#include "struct/stack.h"

#include <string.h>

#include "nio/mem.h"
#include "oop/stride.h"
#include "oop/type.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Stack
 * ============================================================================
 * LIFO stack ported from struct/Stack.java: embeds the Collection mirror as
 * its first member (a Stack* is a Collection*) and grows its single data
 * buffer by doubling when full, copying live entries into the new arena block.
 * Push/pop/peek/slot ride Collection_readSlot / Collection_writeSlot; capacity
 * floors at 1024. Doubling matches the sibling containers and keeps the total
 * bump allocated across growths proportional to the final size, because freed
 * bump storage is reclaimed only wholesale. Lives at R2 as a leaf container
 * behavior.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Stack (struct/stack.c)
 * ============================================================================
 * the Stack class, ported from struct/Stack.java. LIFO over a
 *
 * STRUCT FIELDS (Mirroring struct/stack.h):
 * ----------------------------------------------------------------------------
 *   Stack {
 *     Collection collection; // instance state
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Stack_2(element_class, capacity)
 *   - Stack_1(element_class)
 *
 * Core Functions:
 *   - Stack_2Count(element_class, count)
 *   - Stack_free(stack)
 *   - Stack_push(stack, value_or_pointer)
 *   - Stack_pushTry(stack, value_or_pointer)
 *   - Stack_pop(stack)
 *   - Stack_peek(stack)
 *   - Stack_slot(stack, index)
 *   - Stack_size(stack)
 *   - Stack_length(stack)
 *   - Stack_capacity(stack)
 *   - Stack_elementClassId(stack)
 *   - Stack_stride(stack)
 *   - Stack_dataBuffer(stack)
 *
 * Getters:
 *   - Stack_isEmpty(stack)
 * ============================================================================
 */


// stack.c — Stack port (Legacy: struct/Stack.java).

static const size_t DEFAULT_CAPACITY = 1024;

static Collection *asCollection(Stack *stack) {
    return (Collection*) stack;
}

static Stack *instant(uint32_t elementClass, size_t capacity, size_t count) {
    size_t stride = Stride_get(elementClass);
    size_t cap = capacity < DEFAULT_CAPACITY ? DEFAULT_CAPACITY : capacity;
    Stack *stack = (Stack*) Memory_alloc(TYPE_STACK, sizeof(Stack));
    if (!stack)
        return nullptr;

    Collection *c = asCollection(stack);
    (*c).typeId = TYPE_STACK;
    (*c).activeCount = (uint32_t)count;
    (*c).elementClass = elementClass;
    (*c).stride = (uint32_t)stride;
    (*c).capacity = (uint32_t)cap;
    (*c).head = 0;
    (*c).exhaustionCount = 0;
    (*c).exhaustionReported = false;

    size_t Bytes = cap * stride;
    uint64_t bufType = Type_make(PROJ_VEXSPOKE, FORM_ARRAY, elementClass);
    (*c).data = (uint8_t*) Memory_alloc(bufType, Bytes);
    if (!(*c).data) {
        Memory_free(stack);
        return nullptr;
    }
    memset((*c).data, 0, Bytes);
    return stack;
}

Stack *Stack_2(uint32_t elementClass, size_t capacity) {
    return instant(elementClass, capacity, 0);
}

Stack *Stack_2Count(uint32_t elementClass, size_t count) {
    return instant(elementClass, count, count);
}

void Stack_free(Stack *stack) {
    if (!stack) return;
    Collection *c = asCollection(stack);
    if ((*c).data)
        Memory_free((*c).data);
    Memory_free(stack);
}

void Stack_push(Stack *stack, uint64_t valueOrPointer) {
    // Family-consistent void entry point, but not silent: a refused element is
    // reported by Stack_pushTry (THROW + a named TryCode).
    (void) Stack_pushTry(stack, valueOrPointer);
}

TryValue Stack_pushTry(Stack *stack, uint64_t valueOrPointer) {
    if (!stack) {
        THROW("stack pushTry: null stack");
        return TryValue_error(TRY_NULL_ARG);
    }
    Collection *c = asCollection(stack);
    if ((*c).activeCount >= (*c).capacity) {
        // Double, like the sibling containers (Deque/Queue/List). Growing by a
        // fixed step allocates a fresh buffer on every growth, and freed bump
        // storage is reclaimed only wholesale -- so a large push run exhausts
        // the arena long before the buffer fits (the No Hardcoding Law).
        size_t newCap = (*c).capacity ? (size_t)(*c).capacity * 2 : DEFAULT_CAPACITY;
        if (newCap > UINT32_MAX) {
            THROW("stack pushTry: capacity %zu would exceed uint32", newCap);
            return TryValue_error(TRY_OVERFLOW);
        }
        size_t Bytes = newCap * (*c).stride;
        uint64_t bufType = Type_make(PROJ_VEXSPOKE, FORM_ARRAY, (*c).elementClass);
        uint8_t *next = (uint8_t*) Memory_alloc(bufType, Bytes);
        if (!next) {
            // Shared loud, counted refusal (the Exhaustion Loudness Law); the
            // Try carries the machine-readable reason.
            (void) Collection_reportExhaustion(c, Bytes);
            return TryValue_error(TRY_NO_MEMORY);
        }
        memcpy(next, (*c).data, (*c).activeCount * (*c).stride);
        Memory_free((*c).data);
        (*c).data = next;
        (*c).capacity = (uint32_t)newCap;
    }
    Collection_writeSlot(c, (*c).activeCount, valueOrPointer);
    (*c).activeCount++;
    return TryValue_ok(valueOrPointer);
}

uint64_t Stack_pop(Stack *stack) {
    if (!stack) return 0;
    Collection *c = asCollection(stack);
    if ((*c).activeCount == 0)
        return 0;
    (*c).activeCount--;
    return Collection_readSlot(c, (*c).activeCount);
}

uint64_t Stack_peek(Stack *stack) {
    if (!stack) return 0;
    Collection *c = asCollection(stack);
    if ((*c).activeCount == 0)
        return 0;
    return Collection_readSlot(c, (*c).activeCount - 1);
}

uint8_t *Stack_slot(Stack *stack, size_t index) {
    if (!stack) return nullptr;
    Collection *c = asCollection(stack);
    if (index >= (*c).activeCount)
        return nullptr;
    return (*c).data + index * (*c).stride;
}

bool Stack_isEmpty(Stack *stack) {
    return Collection_isEmpty(asCollection(stack));
}

size_t Stack_size(Stack *stack) {
    return Collection_size(asCollection(stack));
}

;;TEST
uint64_t Stack_exhaustionCount(const Stack *stack) {
    return Collection_exhaustionCount((const Collection*) stack);
}

size_t Stack_length(Stack *stack) {
    return Collection_length(asCollection(stack));
}

size_t Stack_capacity(Stack *stack) {
    return Collection_capacity(asCollection(stack));
}

uint32_t Stack_elementClassId(Stack *stack) {
    return Collection_elementClassId(asCollection(stack));
}

size_t Stack_stride(Stack *stack) {
    return Collection_stride(asCollection(stack));
}

;;TEST
uint8_t *Stack_dataBuffer(Stack *stack) {
    return Collection_dataBuffer(asCollection(stack));
}
Stack *Stack_1(uint32_t element_class) {
    return Stack_2(element_class, 16);
}
