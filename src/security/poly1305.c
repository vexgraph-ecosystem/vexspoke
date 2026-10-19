#include "security/poly1305.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "exception/throw.h"

;;DEFINITION
/** Poly1305 is a one-time message authenticator over the prime 2^130 - 5. The
 * 32-byte key is split into a clamped multiplier r and an additive pad s; the
 * accumulator is (acc + block + 2^128) * r mod (2^130 - 5) for every 16-byte
 * block, and the tag is acc + s mod 2^128. It is a one-time key: authenticating
 * two messages under one key lets an attacker forge a third, which is exactly
 * why the AEAD module derives a fresh key per message from ChaCha20.
 *
 * This is the portable 26-bit-limb form, not a table-driven one: all arithmetic
 * is word adds and multiplies, no lookup. Streaming and zero-allocation. The
 * owner test proves the RFC 8439 section 2.5.2 vector; a wrong Poly1305 still
 * returns 16 plausible bytes. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Poly1305 (security/poly1305.c)
 * STRUCT FIELDS (Mirroring security/poly1305.h)
 *   uint32_t r[5]     clamped multiplier, 26-bit limbs
 *   uint32_t h[5]     accumulator, 26-bit limbs
 *   uint32_t pad[4]   additive pad s, four little-endian words
 *   uint8_t  buffer[16] pending block
 *   size_t   leftover bytes buffered
 *   bool     final    the padded final block has been formed
 * PRIVATE HELPERS
 *   load32le(p) / store32le(p, v)   little-endian word access
 *   blocks(self, m, bytes)          process whole 16-byte blocks
 * FUNCTION REGISTRY (exports)
 *   Poly1305_init(self, key)
 *   Poly1305_update(self, data, len)
 *   Poly1305_final(self, tag)
 *   Poly1305(key, data, len, tag)   one-shot
 * CONTRACT: a null self/key/data with a nonzero length rejects with one THROW;
 * no allocation; the state is zeroed on final.
 * ======================================================================== */

static uint32_t load32le(const uint8_t *p) {
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) |
           ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

static void store32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t) v;
    p[1] = (uint8_t) (v >> 8);
    p[2] = (uint8_t) (v >> 16);
    p[3] = (uint8_t) (v >> 24);
}

void Poly1305_init(Poly1305 *self, const uint8_t key[POLY1305_KEY_SIZE]) {
    if (self == nullptr || key == nullptr) {
        THROW("Poly1305_init: null state or key");
        return;
    }
    memset(self, 0, sizeof *self);
    (*self).r[0] = load32le(key) & 0x3ffffffu;
    (*self).r[1] = (load32le(key + 3) >> 2) & 0x3ffff03u;
    (*self).r[2] = (load32le(key + 6) >> 4) & 0x3ffc0ffu;
    (*self).r[3] = (load32le(key + 9) >> 6) & 0x3f03fffu;
    (*self).r[4] = (load32le(key + 12) >> 8) & 0x00fffffu;
    for (unsigned i = 0; i < 4u; ++i)
        (*self).pad[i] = load32le(key + 16u + 4u * i);
}

static void blocks(Poly1305 *self, const uint8_t *m, size_t bytes) {
    uint32_t r0 = (*self).r[0], r1 = (*self).r[1], r2 = (*self).r[2];
    uint32_t r3 = (*self).r[3], r4 = (*self).r[4];
    uint32_t s1 = r1 * 5u, s2 = r2 * 5u, s3 = r3 * 5u, s4 = r4 * 5u;
    uint32_t h0 = (*self).h[0], h1 = (*self).h[1], h2 = (*self).h[2];
    uint32_t h3 = (*self).h[3], h4 = (*self).h[4];
    uint32_t hibit = (*self).final ? 0u : (1u << 24);

    while (bytes >= POLY1305_BLOCK_SIZE) {
        h0 += load32le(m) & 0x3ffffffu;
        h1 += (load32le(m + 3) >> 2) & 0x3ffffffu;
        h2 += (load32le(m + 6) >> 4) & 0x3ffffffu;
        h3 += (load32le(m + 9) >> 6) & 0x3ffffffu;
        h4 += (load32le(m + 12) >> 8) | hibit;

        uint64_t d0 = (uint64_t) h0 * r0 + (uint64_t) h1 * s4 + (uint64_t) h2 * s3 +
                      (uint64_t) h3 * s2 + (uint64_t) h4 * s1;
        uint64_t d1 = (uint64_t) h0 * r1 + (uint64_t) h1 * r0 + (uint64_t) h2 * s4 +
                      (uint64_t) h3 * s3 + (uint64_t) h4 * s2;
        uint64_t d2 = (uint64_t) h0 * r2 + (uint64_t) h1 * r1 + (uint64_t) h2 * r0 +
                      (uint64_t) h3 * s4 + (uint64_t) h4 * s3;
        uint64_t d3 = (uint64_t) h0 * r3 + (uint64_t) h1 * r2 + (uint64_t) h2 * r1 +
                      (uint64_t) h3 * r0 + (uint64_t) h4 * s4;
        uint64_t d4 = (uint64_t) h0 * r4 + (uint64_t) h1 * r3 + (uint64_t) h2 * r2 +
                      (uint64_t) h3 * r1 + (uint64_t) h4 * r0;

        uint32_t c = (uint32_t) (d0 >> 26);
        h0 = (uint32_t) d0 & 0x3ffffffu;
        d1 += c; c = (uint32_t) (d1 >> 26); h1 = (uint32_t) d1 & 0x3ffffffu;
        d2 += c; c = (uint32_t) (d2 >> 26); h2 = (uint32_t) d2 & 0x3ffffffu;
        d3 += c; c = (uint32_t) (d3 >> 26); h3 = (uint32_t) d3 & 0x3ffffffu;
        d4 += c; c = (uint32_t) (d4 >> 26); h4 = (uint32_t) d4 & 0x3ffffffu;
        h0 += c * 5u; c = h0 >> 26; h0 &= 0x3ffffffu;
        h1 += c;

        m += POLY1305_BLOCK_SIZE;
        bytes -= POLY1305_BLOCK_SIZE;
    }

    (*self).h[0] = h0;
    (*self).h[1] = h1;
    (*self).h[2] = h2;
    (*self).h[3] = h3;
    (*self).h[4] = h4;
}

void Poly1305_update(Poly1305 *self, const uint8_t *data, size_t len) {
    if (self == nullptr)
        return;
    if (len == 0u)
        return;
    if (data == nullptr) {
        THROW("Poly1305_update: null data with a nonzero length");
        return;
    }
    if ((*self).leftover > 0u) {
        size_t want = POLY1305_BLOCK_SIZE - (*self).leftover;
        if (want > len)
            want = len;
        memcpy((*self).buffer + (*self).leftover, data, want);
        data += want;
        len -= want;
        (*self).leftover += want;
        if ((*self).leftover < POLY1305_BLOCK_SIZE)
            return;
        blocks(self, (*self).buffer, POLY1305_BLOCK_SIZE);
        (*self).leftover = 0u;
    }
    if (len >= POLY1305_BLOCK_SIZE) {
        size_t whole = len & ~(size_t) (POLY1305_BLOCK_SIZE - 1u);
        blocks(self, data, whole);
        data += whole;
        len -= whole;
    }
    if (len > 0u) {
        memcpy((*self).buffer + (*self).leftover, data, len);
        (*self).leftover += len;
    }
}

bool Poly1305_final(Poly1305 *self, uint8_t tag[POLY1305_TAG_SIZE]) {
    if (self == nullptr || tag == nullptr) {
        THROW("Poly1305_final: null state or tag");
        return false;
    }
    if ((*self).leftover > 0u) {
        (*self).buffer[(*self).leftover] = 1u;
        for (size_t i = (*self).leftover + 1u; i < POLY1305_BLOCK_SIZE; ++i)
            (*self).buffer[i] = 0u;
        (*self).final = true;
        blocks(self, (*self).buffer, POLY1305_BLOCK_SIZE);
    }

    uint32_t h0 = (*self).h[0], h1 = (*self).h[1], h2 = (*self).h[2];
    uint32_t h3 = (*self).h[3], h4 = (*self).h[4];
    uint32_t c;
    c = h1 >> 26; h1 &= 0x3ffffffu;
    h2 += c; c = h2 >> 26; h2 &= 0x3ffffffu;
    h3 += c; c = h3 >> 26; h3 &= 0x3ffffffu;
    h4 += c; c = h4 >> 26; h4 &= 0x3ffffffu;
    h0 += c * 5u; c = h0 >> 26; h0 &= 0x3ffffffu;
    h1 += c;

    uint32_t g0 = h0 + 5u; c = g0 >> 26; g0 &= 0x3ffffffu;
    uint32_t g1 = h1 + c; c = g1 >> 26; g1 &= 0x3ffffffu;
    uint32_t g2 = h2 + c; c = g2 >> 26; g2 &= 0x3ffffffu;
    uint32_t g3 = h3 + c; c = g3 >> 26; g3 &= 0x3ffffffu;
    uint32_t g4 = h4 + c - (1u << 26);

    uint32_t mask = (uint32_t) (g4 >> 31) - 1u;
    g0 &= mask; g1 &= mask; g2 &= mask; g3 &= mask; g4 &= mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0;
    h1 = (h1 & mask) | g1;
    h2 = (h2 & mask) | g2;
    h3 = (h3 & mask) | g3;
    h4 = (h4 & mask) | g4;

    h0 = (h0 | (h1 << 26)) & 0xffffffffu;
    h1 = ((h1 >> 6) | (h2 << 20)) & 0xffffffffu;
    h2 = ((h2 >> 12) | (h3 << 14)) & 0xffffffffu;
    h3 = ((h3 >> 18) | (h4 << 8)) & 0xffffffffu;

    uint64_t f = (uint64_t) h0 + (*self).pad[0]; h0 = (uint32_t) f;
    f = (uint64_t) h1 + (*self).pad[1] + (f >> 32); h1 = (uint32_t) f;
    f = (uint64_t) h2 + (*self).pad[2] + (f >> 32); h2 = (uint32_t) f;
    f = (uint64_t) h3 + (*self).pad[3] + (f >> 32); h3 = (uint32_t) f;

    store32le(tag, h0);
    store32le(tag + 4, h1);
    store32le(tag + 8, h2);
    store32le(tag + 12, h3);
    memset(self, 0, sizeof *self);
    return true;
}

bool Poly1305_tag(const uint8_t key[POLY1305_KEY_SIZE], const uint8_t *data, size_t len,
                  uint8_t tag[POLY1305_TAG_SIZE]) {
    Poly1305 ctx = {0};
    Poly1305_init(&ctx, key);
    Poly1305_update(&ctx, data, len);
    return Poly1305_final(&ctx, tag);
}
