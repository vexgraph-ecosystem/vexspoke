#ifndef OBJECTS_LOCAL_H
#define OBJECTS_LOCAL_H

#include <stdint.h>
#include <stddef.h>
#include "c23/constructor.h"

// objects/local.h — Thread-local variable slot table object wrapper.
// Ported from legacy objects/Local.java. The slot table is arena-backed
// and grows exponentially on demand (the Dynamic Scalability &
// Anti-Hardcoding Law) — thread ids are never capped.

typedef struct Local Local;

// Allocate a new thread-local slot table
Local *Local_0(void);

// Free local memory

Local *Local_2(const Local *init, size_t count);

#define Local(...) CONSTRUCTOR_DISPATCH(Local, __VA_ARGS__)

void Local_free(Local *local);

// Slot accessors per thread id — reads outside the live table return 0,
// writes grow the table first.
uint64_t Local_get(const Local *local, uint32_t threadId);
void     Local_set(Local *local, uint32_t threadId, uint64_t value);

#endif
