#ifndef NIO_RELATIONAL_MEMORY_H
#define NIO_RELATIONAL_MEMORY_H
/* Opt-in extern handshake, NOT the default Memory_* allocator.
 * Supply relational-engine/rust/include and link its resident static library.
 * Rust owns atomic storage; never reinterpret it as C _Atomic objects.
 * R1 keeps the engine and owners alive across consumer hot reloads. Quiesce
 * users before owner destruction. Record schema migration is not implemented.
 * Ordinary Vexspoke builds do not include/link this optional boundary. */
#include "relational_engine/memory.h"
#endif
