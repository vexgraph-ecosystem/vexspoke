#ifndef SECURITY_ARGON2_H
#define SECURITY_ARGON2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/argon2.h — Argon2id (RFC 9106), the memory-hard password KDF.
//
// Argon2id is the modern password hash: slow in time AND expensive in memory, so
// a GPU or ASIC cannot run thousands of guesses in parallel. That is the whole
// difference from PBKDF2, which only costs time. It is built from BLAKE2b
// (security/blake2b.h), which is why that primitive ships first.
//
// This is a MODULE, not a class: it owns no object, only a cold derivation. The
// work memory (m' blocks of 1024 bytes) is allocated for the call and wiped
// before it is released. Correctness is proved by the RFC 9106 test vector in
// tests/vexspoke/security/argon2_test.c; a wrong Argon2 still returns a
// plausible 32-byte tag, so only the published vector counts.
//
// The caller chooses the parameters. OWASP's current minimum for Argon2id is
// about 19 MiB of memory, 2 passes, 1 lane; the named defaults below are a
// starting point, never a ceiling.

#define ARGON2_VERSION_13     0x13u   // RFC 9106 version (19)
#define ARGON2_TYPE_ID        2u      // Argon2id
#define ARGON2_SYNC_POINTS    4u
#define ARGON2_BLOCK_BYTES    1024u
#define ARGON2_SALT_MIN       8u
#define ARGON2_TAG_MIN        4u
#define ARGON2_TAG_MAX        64u

#define ARGON2_MEMORY_DEFAULT 19456u  // 19 MiB, OWASP minimum
#define ARGON2_PASSES_DEFAULT 2u
#define ARGON2_LANES_DEFAULT  1u
#define ARGON2_TAG_DEFAULT    32u

typedef struct Argon2Params {
    uint32_t memoryKiB; // m, memory size in kibibytes
    uint32_t passes;    // t, number of passes
    uint32_t lanes;     // p, degree of parallelism
    uint32_t tagBytes;  // T, desired tag length (4..64)
} Argon2Params;

// Derive a tag. `secret` (K) and `associated` (X) are optional and may be null
// only when their length is zero; `salt` must be at least ARGON2_SALT_MIN bytes.
// `tag` receives (*params).tagBytes bytes and must have room for them. False
// (with one THROW) on invalid input or refused work memory; nothing is written
// on failure.
bool Argon2id_derive(const Argon2Params *params,
                     const uint8_t *password, size_t passwordLen,
                     const uint8_t *salt, size_t saltLen,
                     const uint8_t *secret, size_t secretLen,
                     const uint8_t *associated, size_t associatedLen,
                     uint8_t *tag, size_t tagCap);

#endif
