#ifndef SECURITY_CRYPTO_H
#define SECURITY_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/crypto.h — Zero-Allocation Cryptographic & Hashing Engine.
//
// Features:
// 1. SHA-256 (NIST FIPS 180-4): Standard cryptographic digest (32 bytes).
// 2. Relational Fast Hash: 64-bit and 32-bit non-cryptographic symbol hashing.
// 3. Constant-Time Verification: Side-channel-safe equality check.
// 4. PRNG Engine: XorShift128+ pseudo-random byte and integer generator.
// 5. Hex Encoding/Decoding: Zero-allocation string serialization.

#define CRYPTO_SHA256_DIGEST_SIZE 32
#define CRYPTO_SHA256_HEX_SIZE    65 // 64 chars + null terminator

// State container for streaming SHA-256
typedef struct CryptoSha256 {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buffer[64];
} CryptoSha256;

// Streaming SHA-256 API
void Crypto_sha256Init(CryptoSha256 *ctx);
void Crypto_sha256Update(CryptoSha256 *ctx, const void *data, size_t len);
void Crypto_sha256Final(CryptoSha256 *ctx, uint8_t outDigest[CRYPTO_SHA256_DIGEST_SIZE]);

// One-shot SHA-256 digest (Dest-last order: fn(src, len, dest))
void Crypto_sha256(const void *data, size_t len, uint8_t outDigest[CRYPTO_SHA256_DIGEST_SIZE]);

// One-shot SHA-256 formatted as a 64-character lowercase hex string
void Crypto_sha256Hex(const void *data, size_t len, char outHex[CRYPTO_SHA256_HEX_SIZE]);

// Constant-time memory equality check (returns true iff memory matches; immune to timing attacks)
bool Crypto_constantTimeEquals(const void *a, const void *b, size_t len);

// Relational fast hashing (non-cryptographic, 64-bit & 32-bit mixer for symbols/keys)
uint64_t Crypto_hash64(const void *data, size_t len);
uint32_t Crypto_hash32(const void *data, size_t len);

// PRNG (XorShift128+) state
typedef struct CryptoRng {
    uint64_t s[2];
} CryptoRng;

void     Crypto_rngInit(CryptoRng *rng, uint64_t seed);
uint64_t Crypto_rngNextU64(CryptoRng *rng);
void     Crypto_rngBytes(CryptoRng *rng, void *dest, size_t len);

// Global default PRNG
void     Crypto_randomSeed(uint64_t seed);
uint64_t Crypto_randomU64(void);
void     Crypto_randomBytes(void *dest, size_t len);

// Hex helpers (Dest-last order)
void   Crypto_toHex(const uint8_t *bytes, size_t len, char *outHex);
size_t Crypto_fromHex(const char *hex, uint8_t *outBytes, size_t maxBytes);

#endif
