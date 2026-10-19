#include "security/blake2b.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "exception/throw.h"

;;DEFINITION
/** BLAKE2b is the hash Argon2 is built from, so it is the first stone of a
 * self-contained Argon2id: RFC 9106 specifies Argon2 in terms of BLAKE2b, both
 * as its H' variable-length hash and as the basis of the G compression
 * function. Shipping Argon2 without BLAKE2b would mean shipping half a
 * specification.
 *
 * It is a streaming digest with a fixed digest length chosen at init, and an
 * optional key that turns it into a MAC. The whole state is 128 bytes of
 * buffer, a 128-bit byte counter, eight 64-bit words and two lengths — no heap,
 * no allocation, no logging. Correctness is not asserted here; it is proven
 * against the RFC 7693 vector and the official BLAKE2 KATs by the owner test,
 * because a wrong BLAKE2b still returns 64 plausible bytes. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Blake2b (security/blake2b.c)
 * STRUCT FIELDS (Mirroring security/blake2b.h)
 *   uint64_t h[8]     working hash words
 *   uint64_t t[2]     128-bit byte counter (low, high)
 *   uint8_t  buf[128] pending block
 *   size_t   used     bytes buffered in buf
 *   size_t   outlen   digest length chosen at init
 * FUNCTION REGISTRY (exports)
 *   Blake2b_init(self, outlen) / Blake2b_initKeyed(self, outlen, key, keylen)
 *   Blake2b_update(self, data, len)
 *   Blake2b_final(self, out, outCap)
 *   Blake2b_digest(data, len, outlen, out, outCap)   one-shot unkeyed
 * PRIVATE HELPERS
 *   BLAKE2B_IV[8]         the RFC 7693 IV
 *   BLAKE2B_SIGMA[12][16] the message schedule
 *   rotr64(v, bits)       right rotation
 *   mix(v, a, b, c, d, x, y)   the G mixing function
 *   compress(self, block, last)  one 128-byte compression
 * CONTRACT: init validates outlen/keylen and THROWs; update accepts any length;
 * final THROWs on a short destination and zeroes the context. No allocation, no
 * logging beyond the cold THROW.
 * ======================================================================== */

static const uint64_t BLAKE2B_IV[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
};

static const uint8_t BLAKE2B_SIGMA[12][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
    { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
    { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
    { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
    { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
    { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
    { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
    { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
};

static uint64_t rotr64(uint64_t v, unsigned bits) {
    return (v >> bits) | (v << (64u - bits));
}

static void mix(uint64_t *v, unsigned a, unsigned b, unsigned c, unsigned d,
                uint64_t x, uint64_t y) {
    v[a] = v[a] + v[b] + x;
    v[d] = rotr64(v[d] ^ v[a], 32u);
    v[c] = v[c] + v[d];
    v[b] = rotr64(v[b] ^ v[c], 24u);
    v[a] = v[a] + v[b] + y;
    v[d] = rotr64(v[d] ^ v[a], 16u);
    v[c] = v[c] + v[d];
    v[b] = rotr64(v[b] ^ v[c], 63u);
}

static void compress(Blake2b *self, const uint8_t *block, bool last) {
    uint64_t m[16];
    for (unsigned i = 0; i < 16u; ++i) {
        m[i] = 0u;
        for (unsigned j = 0; j < 8u; ++j)
            m[i] |= (uint64_t) block[i * 8u + j] << (8u * j);
    }
    uint64_t v[16];
    for (unsigned i = 0; i < 8u; ++i) {
        v[i] = (*self).h[i];
        v[i + 8u] = BLAKE2B_IV[i];
    }
    v[12] ^= (*self).t[0];
    v[13] ^= (*self).t[1];
    if (last)
        v[14] = ~v[14];
    for (unsigned r = 0; r < 12u; ++r) {
        const uint8_t *s = BLAKE2B_SIGMA[r];
        mix(v, 0u, 4u, 8u, 12u, m[s[0]], m[s[1]]);
        mix(v, 1u, 5u, 9u, 13u, m[s[2]], m[s[3]]);
        mix(v, 2u, 6u, 10u, 14u, m[s[4]], m[s[5]]);
        mix(v, 3u, 7u, 11u, 15u, m[s[6]], m[s[7]]);
        mix(v, 0u, 5u, 10u, 15u, m[s[8]], m[s[9]]);
        mix(v, 1u, 6u, 11u, 12u, m[s[10]], m[s[11]]);
        mix(v, 2u, 7u, 8u, 13u, m[s[12]], m[s[13]]);
        mix(v, 3u, 4u, 9u, 14u, m[s[14]], m[s[15]]);
    }
    for (unsigned i = 0; i < 8u; ++i)
        (*self).h[i] ^= v[i] ^ v[i + 8u];
}

bool Blake2b_init(Blake2b *self, size_t outlen) {
    if (self == nullptr || outlen < BLAKE2B_DIGEST_MIN || outlen > BLAKE2B_DIGEST_MAX) {
        THROW("Blake2b_init: null context or digest length out of range");
        return false;
    }
    memset(self, 0, sizeof *self);
    for (unsigned i = 0; i < 8u; ++i)
        (*self).h[i] = BLAKE2B_IV[i];
    (*self).h[0] ^= (uint64_t) 0x01010000u ^ (uint64_t) outlen;
    (*self).outlen = outlen;
    return true;
}

bool Blake2b_initKeyed(Blake2b *self, size_t outlen, const uint8_t *key, size_t keylen) {
    if (keylen == 0u)
        return Blake2b_init(self, outlen);
    if (self == nullptr || key == nullptr || keylen < BLAKE2B_KEY_MIN ||
        keylen > BLAKE2B_KEY_MAX || outlen < BLAKE2B_DIGEST_MIN ||
        outlen > BLAKE2B_DIGEST_MAX) {
        THROW("Blake2b_initKeyed: null context/key or length out of range");
        return false;
    }
    if (!Blake2b_init(self, outlen))
        return false;
    (*self).h[0] ^= (uint64_t) keylen << 8;
    memcpy((*self).buf, key, keylen);
    memset((*self).buf + keylen, 0, BLAKE2B_BLOCK_SIZE - keylen);
    (*self).used = BLAKE2B_BLOCK_SIZE;
    return true;
}

void Blake2b_update(Blake2b *self, const void *data, size_t len) {
    if (self == nullptr)
        return;
    if (len == 0u)
        return;
    if (data == nullptr) {
        THROW("Blake2b_update: null data with a nonzero length");
        return;
    }
    const uint8_t *p = (const uint8_t*) data;
    if ((*self).used + len > BLAKE2B_BLOCK_SIZE) {
        size_t take = BLAKE2B_BLOCK_SIZE - (*self).used;
        memcpy((*self).buf + (*self).used, p, take);
        (*self).t[0] += BLAKE2B_BLOCK_SIZE;
        if ((*self).t[0] < BLAKE2B_BLOCK_SIZE)
            (*self).t[1]++;
        compress(self, (*self).buf, false);
        (*self).used = 0u;
        p += take;
        len -= take;
        while (len > BLAKE2B_BLOCK_SIZE) {
            (*self).t[0] += BLAKE2B_BLOCK_SIZE;
            if ((*self).t[0] < BLAKE2B_BLOCK_SIZE)
                (*self).t[1]++;
            compress(self, p, false);
            p += BLAKE2B_BLOCK_SIZE;
            len -= BLAKE2B_BLOCK_SIZE;
        }
    }
    memcpy((*self).buf + (*self).used, p, len);
    (*self).used += len;
}

bool Blake2b_final(Blake2b *self, uint8_t *out, size_t outCap) {
    if (self == nullptr || out == nullptr || outCap < (*self).outlen) {
        THROW("Blake2b_final: null context/output or short destination");
        return false;
    }
    (*self).t[0] += (*self).used;
    if ((*self).t[0] < (*self).used)
        (*self).t[1]++;
    memset((*self).buf + (*self).used, 0, BLAKE2B_BLOCK_SIZE - (*self).used);
    compress(self, (*self).buf, true);
    for (size_t i = 0; i < (*self).outlen; ++i)
        out[i] = (uint8_t) ((*self).h[i / 8u] >> (8u * (i % 8u)));
    memset(self, 0, sizeof *self);
    return true;
}

bool Blake2b_digest(const void *data, size_t len, size_t outlen, uint8_t *out, size_t outCap) {
    Blake2b ctx = Blake2b_0();
    if (!Blake2b_init(&ctx, outlen))
        return false;
    Blake2b_update(&ctx, data, len);
    return Blake2b_final(&ctx, out, outCap);
}
