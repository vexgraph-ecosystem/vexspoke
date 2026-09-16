#ifndef C23_FREE_H
#define C23_FREE_H
#include <stdint.h>

// c23/free.h — The Relational Destructor Dispatcher.
// 
// In vex, everything is a pointer and every block carries its type in a header.
// This function overloads the standard free(void*), automatically routing to the 
// correct destructor (e.g. Probable_free) based on the runtime type ID, before 
// actually reclaiming the memory block via Memory_free.

typedef void (*DestructorFn)(void *ptr);

// Register a custom destructor hook for a specific 64-bit type ID.
void Destructor_register(uint64_t typeId, DestructorFn fn);
DestructorFn Destructor_lookup(uint64_t typeId);

void c23_free(void *ptr);

// Hijack the standard free call.
// Note: nio/mem.c must NOT include this file (or must #undef free), 
// because Memory_free needs to call the real libc free() to release the block.
#define free(ptr) c23_free(ptr)

#endif // C23_FREE_H
