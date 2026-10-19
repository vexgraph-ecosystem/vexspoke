#ifndef SECURITY_CHACHA20_H
#define SECURITY_CHACHA20_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/chacha20.h — ChaCha20 (RFC 8439), the stream cipher under the AEAD.
//
// ChaCha20 turns a 32-byte key, a 96-bit nonce and a 32-bit block counter into a
// keystream; XOR it with plaintext to encrypt. It is a constant-time ARX design
// (add, rotate, XOR) with no data-dependent table lookups, which is why it is the
// portable choice where AES hardware is absent. It is here so that ChaCha20-
// Poly1305 can be built from pieces this tree owns.
//
// The nonce must never repeat under one key. That is the caller's contract; the
// AEAD module is what makes it hard to get wrong.
//
// Correctness is proved by the RFC 8439 section 2.3.2 block vector and the
// section 2.4.2 encryption vector in tests/vexspoke/security/chacha20_test.c.

#define CHACHA20_KEY_SIZE   32u
#define CHACHA20_NONCE_SIZE 12u
#define CHACHA20_BLOCK_SIZE 64u

// One 64-byte keystream block. out, key and nonce are borrowed.
void ChaCha20_block(uint8_t out[CHACHA20_BLOCK_SIZE], const uint8_t key[CHACHA20_KEY_SIZE],
                    uint32_t counter, const uint8_t nonce[CHACHA20_NONCE_SIZE]);
// XOR the keystream over `len` bytes of data in place, starting at `counter`.
void ChaCha20_xor(uint8_t *data, size_t len, const uint8_t key[CHACHA20_KEY_SIZE],
                  uint32_t counter, const uint8_t nonce[CHACHA20_NONCE_SIZE]);

#endif
