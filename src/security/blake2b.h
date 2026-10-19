#ifndef SECURITY_BLAKE2B_H
#define SECURITY_BLAKE2B_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"

// security/blake2b.h — BLAKE2b (RFC 7693), the hash Argon2 is built on.
//
// BLAKE2b is a fast cryptographic hash, unkeyed or keyed. It is here for one
// reason above all: RFC 9106 builds Argon2 from BLAKE2b, so a self-contained
// Argon2id needs a self-contained BLAKE2b first. Keyed BLAKE2b is also a fine
// MAC, but HMAC-SHA256 stays the interoperable one for TOTP and webhooks.
//
// Streaming, zero-allocation, deterministic. The digest length is fixed at init
// (1..64 bytes) and the key, when present, is 1..64 bytes. No heap, no logging.
// Correctness is proved by the RFC 7693 vector and the official BLAKE2 KATs in
// tests/vexspoke/security/blake2b_test.c, not by "it produced 64 bytes".

#define BLAKE2B_BLOCK_SIZE      128u
#define BLAKE2B_DIGEST_MIN        1u
#define BLAKE2B_DIGEST_MAX       64u
#define BLAKE2B_DIGEST_DEFAULT   64u
#define BLAKE2B_KEY_MIN           1u
#define BLAKE2B_KEY_MAX          64u

typedef struct Blake2b {
    uint64_t h[8];                     // working hash words
    uint64_t t[2];                     // 128-bit byte counter
    uint8_t  buf[BLAKE2B_BLOCK_SIZE];  // pending block
    size_t   used;                     // bytes buffered
    size_t   outlen;                   // digest length chosen at init
} Blake2b;

static inline Blake2b Blake2b_0(void) { return (Blake2b) {0}; }
#define Blake2b(...) CONSTRUCTOR_DISPATCH(Blake2b, __VA_ARGS__)

// Init unkeyed. outlen in [1,64]. False (with THROW) on a null self or bad outlen.
bool Blake2b_init(Blake2b *self, size_t outlen);
// Init keyed (a MAC). keylen 0 is the same as init; key may be null only then.
bool Blake2b_initKeyed(Blake2b *self, size_t outlen, const uint8_t *key, size_t keylen);

// Absorb `len` bytes. A null self is a no-op; null data with a nonzero len rejects.
void Blake2b_update(Blake2b *self, const void *data, size_t len);
// Write (*self).outlen bytes into out. False (with THROW) when out is null or
// outCap is shorter than outlen. Zeroes the context on success.
bool Blake2b_final(Blake2b *self, uint8_t *out, size_t outCap);

// One-shot unkeyed digest of `data` (outCap must be >= outlen).
bool Blake2b_digest(const void *data, size_t len, size_t outlen, uint8_t *out, size_t outCap);

#endif
