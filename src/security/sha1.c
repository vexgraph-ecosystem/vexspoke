#include "security/sha1.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/** SHA-1 as a streaming digest. It ships for HMAC-SHA1 in TOTP and for nothing
 * else: collision resistance is broken and irrelevant to an HMAC, where what
 * matters is the compression function acting as a PRF. Zero allocation, no
 * logging, deterministic. */
;;OVERVIEW
/** ========================================================================
 * MODULE: CryptoSha1 (security/sha1.c)
 * STRUCT FIELDS (Mirroring security/sha1.h)
 *   uint32_t state[5]   working hash words
 *   uint8_t  block[64]  pending input block
 *   uint64_t total      message length in bytes
 *   size_t   used       bytes currently buffered
 * FUNCTION REGISTRY (exports)
 *   Crypto_sha1Init(ctx) / Update(ctx, data, len) / Final(ctx, outDigest)
 *   Crypto_sha1(data, len, outDigest)   one-shot
 * PRIVATE HELPERS: sha1Compress(ctx, block) — the 80-round compression
 * CONTRACT: null ctx/data no-ops or rejects; Final zeroes the context;
 * no allocation, no logging.
 * ======================================================================== */

static uint32_t rotr(uint32_t v, unsigned bits) { return (v >> bits) | (v << (32u - bits)); }

static void sha1Compress(CryptoSha1 *ctx, const uint8_t *block) {
    uint32_t w[80];
    for (unsigned i = 0; i < 16u; ++i)
        w[i] = ((uint32_t) block[i * 4] << 24) | ((uint32_t) block[i * 4 + 1] << 16) |
               ((uint32_t) block[i * 4 + 2] << 8) | (uint32_t) block[i * 4 + 3];
    for (unsigned i = 16u; i < 80u; ++i)
        w[i] = rotr(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 31u);
    uint32_t a = (*ctx).state[0], b = (*ctx).state[1], c = (*ctx).state[2];
    uint32_t d = (*ctx).state[3], e = (*ctx).state[4];
    for (unsigned i = 0; i < 80u; ++i) {
        uint32_t f = 0, k = 0;
        if (i < 20u) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999u;
        } else if (i < 40u) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60u) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        uint32_t temp = rotr(a, 27u) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotr(b, 2u);
        b = a;
        a = temp;
    }
    (*ctx).state[0] += a;
    (*ctx).state[1] += b;
    (*ctx).state[2] += c;
    (*ctx).state[3] += d;
    (*ctx).state[4] += e;
}

void Crypto_sha1Init(CryptoSha1 *ctx) {
    if (ctx == nullptr) {
        THROW("Crypto_sha1Init: null context");
        return;
    }
    (*ctx).state[0] = 0x67452301u;
    (*ctx).state[1] = 0xEFCDAB89u;
    (*ctx).state[2] = 0x98BADCFEu;
    (*ctx).state[3] = 0x10325476u;
    (*ctx).state[4] = 0xC3D2E1F0u;
    (*ctx).total = 0u;
    (*ctx).used = 0u;
}

void Crypto_sha1Update(CryptoSha1 *ctx, const void *data, size_t len) {
    if (ctx == nullptr || (data == nullptr && len != 0u)) {
        THROW("Crypto_sha1Update: null input");
        return;
    }
    const uint8_t *bytes = (const uint8_t*) data;
    (*ctx).total += len;
    while (len > 0u) {
        size_t room = CRYPTO_SHA1_BLOCK_SIZE - (*ctx).used;
        size_t take = len < room ? len : room;
        for (size_t i = 0; i < take; ++i)
            (*ctx).block[(*ctx).used + i] = bytes[i];
        (*ctx).used += take;
        bytes += take;
        len -= take;
        if ((*ctx).used == CRYPTO_SHA1_BLOCK_SIZE) {
            sha1Compress(ctx, (*ctx).block);
            (*ctx).used = 0u;
        }
    }
}

void Crypto_sha1Final(CryptoSha1 *ctx, uint8_t outDigest[CRYPTO_SHA1_DIGEST_SIZE]) {
    if (ctx == nullptr || outDigest == nullptr) {
        THROW("Crypto_sha1Final: null argument");
        return;
    }
    uint64_t bits = (*ctx).total * 8u;
    uint8_t pad = 0x80u;
    Crypto_sha1Update(ctx, &pad, 1u);
    uint8_t zero = 0u;
    while ((*ctx).used != CRYPTO_SHA1_BLOCK_SIZE - 8u)
        Crypto_sha1Update(ctx, &zero, 1u);
    uint8_t lengthBytes[8];
    for (unsigned i = 0; i < 8u; ++i)
        lengthBytes[i] = (uint8_t) (bits >> (56u - i * 8u));
    Crypto_sha1Update(ctx, lengthBytes, 8u);
    for (unsigned i = 0; i < 5u; ++i) {
        outDigest[i * 4]     = (uint8_t) ((*ctx).state[i] >> 24);
        outDigest[i * 4 + 1] = (uint8_t) ((*ctx).state[i] >> 16);
        outDigest[i * 4 + 2] = (uint8_t) ((*ctx).state[i] >> 8);
        outDigest[i * 4 + 3] = (uint8_t) (*ctx).state[i];
    }
    for (unsigned i = 0; i < 5u; ++i)
        (*ctx).state[i] = 0u;
    for (unsigned i = 0; i < CRYPTO_SHA1_BLOCK_SIZE; ++i)
        (*ctx).block[i] = 0u;
}

void Crypto_sha1(const void *data, size_t len, uint8_t outDigest[CRYPTO_SHA1_DIGEST_SIZE]) {
    CryptoSha1 ctx;
    Crypto_sha1Init(&ctx);
    Crypto_sha1Update(&ctx, data, len);
    Crypto_sha1Final(&ctx, outDigest);
}
