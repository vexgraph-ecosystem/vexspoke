#ifndef SECURITY_SHA1_H
#define SECURITY_SHA1_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/sha1.h — SHA-1 (FIPS 180-4), the same `Crypto` module family as
// crypto.h, split out per the Single Class Per File Law.
//
// It exists for ONE reason: HMAC-SHA1 is the interoperable default for TOTP
// (RFC 6238), and authenticator apps expect it. SHA-1's collision weakness does
// NOT apply to HMAC — the construction needs a compression function with the
// pseudo-randomness properties of a PRF, not collision resistance — so this is
// not a "broken hash in a security path". Do not silently upgrade it to SHA-256:
// that breaks interoperability with apps that only offer SHA-1.
//
// Streaming, zero-allocation, deterministic. No heap, no logging.

#define CRYPTO_SHA1_DIGEST_SIZE 20u
#define CRYPTO_SHA1_BLOCK_SIZE  64u

typedef struct CryptoSha1 {
    uint32_t state[5];
    uint8_t  block[CRYPTO_SHA1_BLOCK_SIZE];
    uint64_t total;   // message length in bytes
    size_t   used;    // bytes buffered in block
} CryptoSha1;

void Crypto_sha1Init(CryptoSha1 *ctx);
void Crypto_sha1Update(CryptoSha1 *ctx, const void *data, size_t len);
void Crypto_sha1Final(CryptoSha1 *ctx, uint8_t outDigest[CRYPTO_SHA1_DIGEST_SIZE]);
void Crypto_sha1(const void *data, size_t len, uint8_t outDigest[CRYPTO_SHA1_DIGEST_SIZE]);

#endif
