#ifndef SECURITY_HMAC_H
#define SECURITY_HMAC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/hmac.h — HMAC (RFC 2104) over the digests this module owns.
//
// HMAC is what TOTP is built from, and it is the reason SHA-1 still has a job in
// this tree. Both variants stream through the existing digest contexts, so a
// message is never copied into a concatenation buffer: the pads are stack arrays
// and the key is scrubbed before return.
//
// Zero allocation. Keys longer than one block are hashed first, as the RFC requires.

#define HMAC_SHA1_DIGEST_SIZE   20u
#define HMAC_SHA1_BLOCK_SIZE    64u
#define HMAC_SHA256_DIGEST_SIZE 32u
#define HMAC_SHA256_BLOCK_SIZE  64u

// All four pointers are borrowed; the key and data may be null only when their
// length is zero. Output goes into the caller's buffer.
void Hmac_sha1(const uint8_t *key, size_t keyLen, const uint8_t *data, size_t dataLen,
               uint8_t out[HMAC_SHA1_DIGEST_SIZE]);
void Hmac_sha256(const uint8_t *key, size_t keyLen, const uint8_t *data, size_t dataLen,
                 uint8_t out[HMAC_SHA256_DIGEST_SIZE]);

#endif
