#ifndef SECURITY_POLY1305_H
#define SECURITY_POLY1305_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/poly1305.h — Poly1305 (RFC 8439), the one-time authenticator.
//
// Poly1305 takes a 32-byte one-time key (16 bytes of r, 16 bytes of s) and
// produces a 16-byte tag over a message. It is a one-time MAC: reusing the key
// on two different messages breaks it, which is why the AEAD derives a fresh key
// from ChaCha20 for every message. It is streaming and zero-allocation.
//
// Correctness is proved by the RFC 8439 section 2.5.2 vector in
// tests/vexspoke/security/poly1305_test.c.

#define POLY1305_KEY_SIZE 32u
#define POLY1305_TAG_SIZE 16u
#define POLY1305_BLOCK_SIZE 16u

typedef struct Poly1305 {
    uint32_t r[5];               // clamped r, 26-bit limbs
    uint32_t h[5];               // accumulator, 26-bit limbs
    uint32_t pad[4];             // s, as four little-endian words
    uint8_t  buffer[POLY1305_BLOCK_SIZE];
    size_t   leftover;           // bytes buffered
    bool     final;              // the padded final block has been formed
} Poly1305;

// Init with a 32-byte one-time key. Reusing the key is a caller contract break.
void Poly1305_init(Poly1305 *self, const uint8_t key[POLY1305_KEY_SIZE]);
void Poly1305_update(Poly1305 *self, const uint8_t *data, size_t len);
// Write the 16-byte tag; zeroes the state on success.
bool Poly1305_final(Poly1305 *self, uint8_t tag[POLY1305_TAG_SIZE]);
// One-shot: init + update + final over `len` bytes.
bool Poly1305_tag(const uint8_t key[POLY1305_KEY_SIZE], const uint8_t *data, size_t len,
                  uint8_t tag[POLY1305_TAG_SIZE]);

#endif
