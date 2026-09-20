#include "security/crypto.h"

#include <string.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Crypto
 * ============================================================================
 * Zero-allocation cryptographic and hashing core: streaming NIST SHA-256
 * (init/update/final plus one-shot and hex forms), constant-time equality
 * immune to timing attacks, FNV-1a + avalanche relational hash mixers, and
 * an XorShift128+ PRNG with a seeded global default. Every function operates
 * on caller-owned state (CryptoSha256, CryptoRng) or stack buffers — no
 * allocation, no globals except the default RNG. All digests and hex output
 * are dest-last per the Dest-Last Law.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Crypto (security/crypto.c)
 * LEVEL: L1 — Cryptographic & Hashing Core
 * ============================================================================
 * Zero-allocation cryptographic digests (NIST SHA-256), side-channel-safe
 * constant-time equality checks, relational hash mixers, and XorShift128+ PRNG.
 *
 * STRUCT FIELDS (Mirroring security/crypto.h):
 * ----------------------------------------------------------------------------
 *   CryptoSha256 {
 *     uint32_t state[8];  // SHA-256 working state (8 x 32-bit words)
 *     uint64_t count;     // total bytes fed (bit length for the final block)
 *     uint8_t buffer[64]; // pending input block
 *   }
 *   CryptoRng {
 *     uint64_t s[2]; // XorShift128+ state words
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - Crypto_sha256Init(ctx)
 *   - Crypto_sha256Update(ctx, data, len)
 *   - Crypto_sha256Final(ctx, outDigest)
 *   - Crypto_sha256(data, len, outDigest)
 *   - Crypto_sha256Hex(data, len, outHex)
 *   - Crypto_constantTimeEquals(a, b, len)
 *   - Crypto_hash64(data, len)
 *   - Crypto_hash32(data, len)
 *   - Crypto_rngInit(rng, seed)
 *   - Crypto_rngNextU64(rng)
 *   - Crypto_rngBytes(rng, dest, len)
 *   - Crypto_randomSeed(seed)
 *   - Crypto_randomU64(void)
 *   - Crypto_randomBytes(dest, len)
 *   - Crypto_toHex(bytes, len, outHex)
 *   - Crypto_fromHex(hex, outBytes, maxBytes)
 *
 * Private Core Functions: (.c static)
 *   - Crypto_rotr(x, n)                  : rotate-right helper
 *   - Crypto_sha256Transform(ctx, data)  : one 64-byte block transform
 *   - splitmix64(seed)                   : PRNG seed expansion
 *   - hexVal(c)                          : hex digit decode
 * ============================================================================
 */

// ============================================================================
// SHA-256 (NIST FIPS 180-4) Implementation
// ============================================================================

static inline uint32_t Crypto_rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (Crypto_rotr(x, 2) ^ Crypto_rotr(x, 13) ^ Crypto_rotr(x, 22))
#define EP1(x)       (Crypto_rotr(x, 6) ^ Crypto_rotr(x, 11) ^ Crypto_rotr(x, 25))
#define SIG0(x)      (Crypto_rotr(x, 7) ^ Crypto_rotr(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (Crypto_rotr(x, 17) ^ Crypto_rotr(x, 19) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static void Crypto_sha256Transform(CryptoSha256 *ctx, const uint8_t data[64]) {
    uint32_t m[64];
    for (int i = 0; i < 16; i++) {
        m[i] = ((uint32_t) data[i * 4] << 24) |
               ((uint32_t) data[i * 4 + 1] << 16) |
               ((uint32_t) data[i * 4 + 2] << 8) |
               ((uint32_t) data[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    uint32_t a = (*ctx).state[0];
    uint32_t b = (*ctx).state[1];
    uint32_t c = (*ctx).state[2];
    uint32_t d = (*ctx).state[3];
    uint32_t e = (*ctx).state[4];
    uint32_t f = (*ctx).state[5];
    uint32_t g = (*ctx).state[6];
    uint32_t h = (*ctx).state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    (*ctx).state[0] += a;
    (*ctx).state[1] += b;
    (*ctx).state[2] += c;
    (*ctx).state[3] += d;
    (*ctx).state[4] += e;
    (*ctx).state[5] += f;
    (*ctx).state[6] += g;
    (*ctx).state[7] += h;
}

void Crypto_sha256Init(CryptoSha256 *ctx) {
    if (ctx == nullptr) return;
    (*ctx).count = 0;
    (*ctx).state[0] = 0x6a09e667u;
    (*ctx).state[1] = 0xbb67ae85u;
    (*ctx).state[2] = 0x3c6ef372u;
    (*ctx).state[3] = 0xa54ff53au;
    (*ctx).state[4] = 0x510e527fu;
    (*ctx).state[5] = 0x9b05688cu;
    (*ctx).state[6] = 0x1f83d9abu;
    (*ctx).state[7] = 0x5be0cd19u;
}

void Crypto_sha256Update(CryptoSha256 *ctx, const void *data, size_t len) {
    if (ctx == nullptr || data == nullptr || len == 0) return;

    const uint8_t *p = (const uint8_t*) data;
    size_t bufIdx = (size_t) ((*ctx).count & 0x3F);
    (*ctx).count += (uint64_t) len;

    // Fill buffer if partially full
    if (bufIdx > 0) {
        size_t needed = 64 - bufIdx;
        if (len < needed) {
            memcpy((*ctx).buffer + bufIdx, p, len);
            return;
        }
        memcpy((*ctx).buffer + bufIdx, p, needed);
        Crypto_sha256Transform(ctx, (*ctx).buffer);
        p += needed;
        len -= needed;
    }

    // Process full 64-byte blocks
    while (len >= 64) {
        Crypto_sha256Transform(ctx, p);
        p += 64;
        len -= 64;
    }

    // Store trailing remainder
    if (len > 0) {
        memcpy((*ctx).buffer, p, len);
    }
}

void Crypto_sha256Final(CryptoSha256 *ctx, uint8_t outDigest[CRYPTO_SHA256_DIGEST_SIZE]) {
    if (ctx == nullptr || outDigest == nullptr) return;

    uint64_t bitCount = (*ctx).count * 8ULL;
    size_t bufIdx = (size_t) ((*ctx).count & 0x3F);

    // Append 0x80 bit
    (*ctx).buffer[bufIdx++] = 0x80;

    if (bufIdx > 56) {
        // Not enough room for 8-byte length in this block
        memset((*ctx).buffer + bufIdx, 0, 64 - bufIdx);
        Crypto_sha256Transform(ctx, (*ctx).buffer);
        memset((*ctx).buffer, 0, 56);
    } else {
        memset((*ctx).buffer + bufIdx, 0, 56 - bufIdx);
    }

    // Append 64-bit big-endian bit length
    for (int i = 7; i >= 0; i--) {
        (*ctx).buffer[56 + (7 - i)] = (uint8_t) ((bitCount >> (i * 8)) & 0xFF);
    }
    Crypto_sha256Transform(ctx, (*ctx).buffer);

    // Output big-endian digest
    for (int i = 0; i < 8; i++) {
        outDigest[i * 4]     = (uint8_t) (((*ctx).state[i] >> 24) & 0xFF);
        outDigest[i * 4 + 1] = (uint8_t) (((*ctx).state[i] >> 16) & 0xFF);
        outDigest[i * 4 + 2] = (uint8_t) (((*ctx).state[i] >> 8) & 0xFF);
        outDigest[i * 4 + 3] = (uint8_t) (((*ctx).state[i]) & 0xFF);
    }
}

void Crypto_sha256(const void *data, size_t len, uint8_t outDigest[CRYPTO_SHA256_DIGEST_SIZE]) {
    CryptoSha256 ctx;
    Crypto_sha256Init(&ctx);
    Crypto_sha256Update(&ctx, data, len);
    Crypto_sha256Final(&ctx, outDigest);
}

void Crypto_sha256Hex(const void *data, size_t len, char outHex[CRYPTO_SHA256_HEX_SIZE]) {
    uint8_t digest[CRYPTO_SHA256_DIGEST_SIZE];
    Crypto_sha256(data, len, digest);
    Crypto_toHex(digest, CRYPTO_SHA256_DIGEST_SIZE, outHex);
}

// ============================================================================
// Constant-Time Equality Verification
// ============================================================================

bool Crypto_constantTimeEquals(const void *a, const void *b, size_t len) {
    if (a == nullptr || b == nullptr) return false;
    const volatile uint8_t *pa = (const volatile uint8_t*) a;
    const volatile uint8_t *pb = (const volatile uint8_t*) b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (pa[i] ^ pb[i]);
    }
    return (diff == 0);
}

// ============================================================================
// Fast Relational Hash Mixers (64-bit & 32-bit)
// ============================================================================

uint64_t Crypto_hash64(const void *data, size_t len) {
    if (data == nullptr || len == 0) return 0;
    const uint8_t *p = (const uint8_t*) data;
    // FNV-1a 64-bit basis
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t) p[i];
        h *= 0x100000001b3ULL;
    }
    // Murmur/Splitmix final avalanche mixer
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h;
}

uint32_t Crypto_hash32(const void *data, size_t len) {
    uint64_t h = Crypto_hash64(data, len);
    return (uint32_t) (h ^ (h >> 32));
}

// ============================================================================
// XorShift128+ PRNG Engine
// ============================================================================

static uint64_t splitmix64(uint64_t *seed) {
    uint64_t z = (*seed += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void Crypto_rngInit(CryptoRng *rng, uint64_t seed) {
    if (rng == nullptr) return;
    if (seed == 0) seed = 0x853c49e6748fea9bULL;
    uint64_t sm = seed;
    (*rng).s[0] = splitmix64(&sm);
    (*rng).s[1] = splitmix64(&sm);
}

uint64_t Crypto_rngNextU64(CryptoRng *rng) {
    if (rng == nullptr) return 0;
    uint64_t s1 = (*rng).s[0];
    const uint64_t s0 = (*rng).s[1];
    (*rng).s[0] = s0;
    s1 ^= s1 << 23;
    (*rng).s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return (*rng).s[1] + s0;
}

void Crypto_rngBytes(CryptoRng *rng, void *dest, size_t len) {
    if (rng == nullptr || dest == nullptr || len == 0) return;
    uint8_t *p = (uint8_t*) dest;
    while (len >= 8) {
        uint64_t val = Crypto_rngNextU64(rng);
        memcpy(p, &val, 8);
        p += 8;
        len -= 8;
    }
    if (len > 0) {
        uint64_t val = Crypto_rngNextU64(rng);
        memcpy(p, &val, len);
    }
}

static CryptoRng g_defaultRng = { { 0x123456789abcdef0ULL, 0xfedcba9876543210ULL } };

void Crypto_randomSeed(uint64_t seed) {
    Crypto_rngInit(&g_defaultRng, seed);
}

uint64_t Crypto_randomU64(void) {
    return Crypto_rngNextU64(&g_defaultRng);
}

void Crypto_randomBytes(void *dest, size_t len) {
    Crypto_rngBytes(&g_defaultRng, dest, len);
}

// ============================================================================
// Hex Helpers
// ============================================================================

static const char HEX_CHARS[] = "0123456789abcdef";

void Crypto_toHex(const uint8_t *bytes, size_t len, char *outHex) {
    if (outHex == nullptr) return;
    if (bytes == nullptr || len == 0) {
        outHex[0] = '\0';
        return;
    }
    for (size_t i = 0; i < len; i++) {
        outHex[i * 2]     = HEX_CHARS[(bytes[i] >> 4) & 0x0F];
        outHex[i * 2 + 1] = HEX_CHARS[bytes[i] & 0x0F];
    }
    outHex[len * 2] = '\0';
}

static int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

size_t Crypto_fromHex(const char *hex, uint8_t *outBytes, size_t maxBytes) {
    if (hex == nullptr || outBytes == nullptr || maxBytes == 0) return 0;
    size_t hexLen = strlen(hex);
    size_t byteCount = hexLen / 2;
    if (byteCount > maxBytes) byteCount = maxBytes;

    for (size_t i = 0; i < byteCount; i++) {
        int hi = hexVal(hex[i * 2]);
        int lo = hexVal(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            return i; // Stop at invalid hex char
        }
        outBytes[i] = (uint8_t) ((hi << 4) | lo);
    }
    return byteCount;
}
