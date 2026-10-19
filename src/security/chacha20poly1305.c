#include "security/chacha20poly1305.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "exception/throw.h"
#include "security/chacha20.h"
#include "security/crypto.h"
#include "security/poly1305.h"

;;DEFINITION
/** ChaCha20-Poly1305 is the AEAD that turns two primitives into one safe
 * operation. ChaCha20 supplies a keystream for confidentiality; Poly1305
 * supplies a tag for integrity; and the Poly1305 key is the first 32 bytes of
 * the ChaCha20 keystream at counter zero, regenerated for every message. That
 * derivation is the whole reason the one-time-key rule of Poly1305 can be
 * honored by a caller who never sees a Poly1305 key.
 *
 * The MAC covers the associated data, then the ciphertext, each padded to a
 * 16-byte boundary, then both lengths as little-endian 64-bit words. That
 * ordering is what makes truncation, reordering and length tampering fail: the
 * tag commits to the exact lengths. Decryption verifies before it decrypts, and
 * a mismatch zeroes the output rather than returning a plausible plaintext.
 *
 * Correctness is proved by the RFC 8439 section 2.8.2 vector; the owner test
 * also forges a tag and flips a ciphertext bit to prove refusal. */
;;OVERVIEW
/** ========================================================================
 * MODULE: ChaCha20Poly1305 (security/chacha20poly1305.c)
 * PRIVATE HELPERS
 *   pad16(ctx, len)          absorb zero padding to the next 16-byte boundary
 *   store64le(p, v)          little-endian 64-bit store
 *   mac(key, nonce, aad, aadLen, ciphertext, len, tag)
 *                            derive the Poly1305 key and tag the message
 * FUNCTION REGISTRY (exports)
 *   ChaCha20Poly1305_encrypt(key, nonce, aad, aadLen, plaintext, len, ciphertext, tag)
 *   ChaCha20Poly1305_decrypt(key, nonce, aad, aadLen, ciphertext, len, tag, plaintext)
 * CONTRACT: null key/nonce/tag, or a null buffer with a nonzero length, rejects
 * with one THROW. Decrypt is constant-time on the tag and fail-closed. No
 * allocation, no logging beyond the cold rejection.
 * ======================================================================== */

static void pad16(Poly1305 *ctx, size_t len) {
    size_t remainder = len % 16u;
    if (remainder != 0u) {
        static const uint8_t zeros[16] = {0};
        Poly1305_update(ctx, zeros, 16u - remainder);
    }
}

static void store64le(uint8_t *p, uint64_t v) {
    for (unsigned i = 0; i < 8u; ++i)
        p[i] = (uint8_t) (v >> (8u * i));
}

static void mac(const uint8_t key[CHACHA20POLY1305_KEY_SIZE],
                const uint8_t nonce[CHACHA20POLY1305_NONCE_SIZE],
                const uint8_t *aad, size_t aadLen,
                const uint8_t *ciphertext, size_t len,
                uint8_t tag[CHACHA20POLY1305_TAG_SIZE]) {
    uint8_t polyKey[CHACHA20_BLOCK_SIZE];
    ChaCha20_block(polyKey, key, 0u, nonce);
    Poly1305 ctx;
    Poly1305_init(&ctx, polyKey);
    if (aadLen > 0u)
        Poly1305_update(&ctx, aad, aadLen);
    pad16(&ctx, aadLen);
    if (len > 0u)
        Poly1305_update(&ctx, ciphertext, len);
    pad16(&ctx, len);
    uint8_t lengths[16];
    store64le(lengths, (uint64_t) aadLen);
    store64le(lengths + 8, (uint64_t) len);
    Poly1305_update(&ctx, lengths, sizeof lengths);
    Poly1305_final(&ctx, tag);
    memset(polyKey, 0, sizeof polyKey);
}

bool ChaCha20Poly1305_encrypt(const uint8_t key[CHACHA20POLY1305_KEY_SIZE],
                              const uint8_t nonce[CHACHA20POLY1305_NONCE_SIZE],
                              const uint8_t *aad, size_t aadLen,
                              const uint8_t *plaintext, size_t plaintextLen,
                              uint8_t *ciphertext, uint8_t tag[CHACHA20POLY1305_TAG_SIZE]) {
    if (key == nullptr || nonce == nullptr || tag == nullptr ||
        (aad == nullptr && aadLen != 0u) ||
        (plaintext == nullptr && plaintextLen != 0u) ||
        (ciphertext == nullptr && plaintextLen != 0u)) {
        THROW("ChaCha20Poly1305_encrypt: null key, nonce, tag or buffer");
        return false;
    }
    if (plaintextLen > 0u)
        memcpy(ciphertext, plaintext, plaintextLen);
    ChaCha20_xor(ciphertext, plaintextLen, key, 1u, nonce);
    mac(key, nonce, aad, aadLen, ciphertext, plaintextLen, tag);
    return true;
}

bool ChaCha20Poly1305_decrypt(const uint8_t key[CHACHA20POLY1305_KEY_SIZE],
                              const uint8_t nonce[CHACHA20POLY1305_NONCE_SIZE],
                              const uint8_t *aad, size_t aadLen,
                              const uint8_t *ciphertext, size_t ciphertextLen,
                              const uint8_t tag[CHACHA20POLY1305_TAG_SIZE],
                              uint8_t *plaintext) {
    if (key == nullptr || nonce == nullptr || tag == nullptr ||
        (aad == nullptr && aadLen != 0u) ||
        (ciphertext == nullptr && ciphertextLen != 0u) ||
        (plaintext == nullptr && ciphertextLen != 0u)) {
        THROW("ChaCha20Poly1305_decrypt: null key, nonce, tag or buffer");
        return false;
    }
    uint8_t expected[CHACHA20POLY1305_TAG_SIZE];
    mac(key, nonce, aad, aadLen, ciphertext, ciphertextLen, expected);
    bool authentic = Crypto_constantTimeEquals(expected, tag, sizeof expected);
    memset(expected, 0, sizeof expected);
    if (!authentic) {
        if (ciphertextLen > 0u)
            memset(plaintext, 0, ciphertextLen);
        THROW("ChaCha20Poly1305_decrypt: authentication tag mismatch");
        return false;
    }
    if (ciphertextLen > 0u)
        memcpy(plaintext, ciphertext, ciphertextLen);
    ChaCha20_xor(plaintext, ciphertextLen, key, 1u, nonce);
    return true;
}
