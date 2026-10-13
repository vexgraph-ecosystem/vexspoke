#include "security/hmac.h"
#include "security/sha1.h"
#include "security/crypto.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/** HMAC as RFC 2104 defines it: H((K ^ opad) || H((K ^ ipad) || message)). Both
 * pads live on the stack, the message is streamed rather than copied, and the key
 * material is scrubbed before returning. Two explicit variants instead of a
 * function-pointer table, because there are exactly two digests in this module and
 * a table would hide that from the reader. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Hmac (security/hmac.c)
 * FUNCTION REGISTRY (exports)
 *   Hmac_sha1(key, keyLen, data, dataLen, out[20])
 *   Hmac_sha256(key, keyLen, data, dataLen, out[32])
 * PRIVATE HELPERS: hmacSha1Block/hmacSha256Block — the shared ipad/opad body
 * CONTRACT: null key/data with nonzero length rejects through THROW; a key longer
 * than the block is hashed first; key pads and inner digests are zeroed before
 * return; no allocation, no logging on success.
 * ======================================================================== */

static void hmacSha1Block(const uint8_t *key, size_t keyLen, const uint8_t *data, size_t dataLen,
                          uint8_t out[HMAC_SHA1_DIGEST_SIZE]) {
    uint8_t pad[HMAC_SHA1_BLOCK_SIZE];
    uint8_t inner[HMAC_SHA1_DIGEST_SIZE];
    uint8_t ipad[HMAC_SHA1_BLOCK_SIZE];
    uint8_t opad[HMAC_SHA1_BLOCK_SIZE];
    for (size_t i = 0; i < HMAC_SHA1_BLOCK_SIZE; ++i)
        pad[i] = 0u;
    if (keyLen > HMAC_SHA1_BLOCK_SIZE)
        Crypto_sha1(key, keyLen, pad);
    else
        for (size_t i = 0; i < keyLen; ++i)
            pad[i] = key[i];
    for (size_t i = 0; i < HMAC_SHA1_BLOCK_SIZE; ++i) {
        ipad[i] = pad[i] ^ 0x36u;
        opad[i] = pad[i] ^ 0x5Cu;
    }
    CryptoSha1 ctx;
    Crypto_sha1Init(&ctx);
    Crypto_sha1Update(&ctx, ipad, HMAC_SHA1_BLOCK_SIZE);
    Crypto_sha1Update(&ctx, data, dataLen);
    Crypto_sha1Final(&ctx, inner);
    Crypto_sha1Init(&ctx);
    Crypto_sha1Update(&ctx, opad, HMAC_SHA1_BLOCK_SIZE);
    Crypto_sha1Update(&ctx, inner, HMAC_SHA1_DIGEST_SIZE);
    Crypto_sha1Final(&ctx, out);
    for (size_t i = 0; i < HMAC_SHA1_BLOCK_SIZE; ++i) {
        pad[i] = 0u;
        ipad[i] = 0u;
        opad[i] = 0u;
    }
    for (size_t i = 0; i < HMAC_SHA1_DIGEST_SIZE; ++i)
        inner[i] = 0u;
}

void Hmac_sha1(const uint8_t *key, size_t keyLen, const uint8_t *data, size_t dataLen,
               uint8_t out[HMAC_SHA1_DIGEST_SIZE]) {
    if ((key == nullptr && keyLen != 0u) || (data == nullptr && dataLen != 0u) || out == nullptr) {
        THROW("Hmac_sha1: null input");
        return;
    }
    hmacSha1Block(key, keyLen, data, dataLen, out);
}

void Hmac_sha256(const uint8_t *key, size_t keyLen, const uint8_t *data, size_t dataLen,
                 uint8_t out[HMAC_SHA256_DIGEST_SIZE]) {
    if ((key == nullptr && keyLen != 0u) || (data == nullptr && dataLen != 0u) || out == nullptr) {
        THROW("Hmac_sha256: null input");
        return;
    }
    uint8_t pad[HMAC_SHA256_BLOCK_SIZE];
    uint8_t inner[HMAC_SHA256_DIGEST_SIZE];
    uint8_t ipad[HMAC_SHA256_BLOCK_SIZE];
    uint8_t opad[HMAC_SHA256_BLOCK_SIZE];
    for (size_t i = 0; i < HMAC_SHA256_BLOCK_SIZE; ++i)
        pad[i] = 0u;
    if (keyLen > HMAC_SHA256_BLOCK_SIZE)
        Crypto_sha256(key, keyLen, pad);
    else
        for (size_t i = 0; i < keyLen; ++i)
            pad[i] = key[i];
    for (size_t i = 0; i < HMAC_SHA256_BLOCK_SIZE; ++i) {
        ipad[i] = pad[i] ^ 0x36u;
        opad[i] = pad[i] ^ 0x5Cu;
    }
    CryptoSha256 ctx;
    Crypto_sha256Init(&ctx);
    Crypto_sha256Update(&ctx, ipad, HMAC_SHA256_BLOCK_SIZE);
    Crypto_sha256Update(&ctx, data, dataLen);
    Crypto_sha256Final(&ctx, inner);
    Crypto_sha256Init(&ctx);
    Crypto_sha256Update(&ctx, opad, HMAC_SHA256_BLOCK_SIZE);
    Crypto_sha256Update(&ctx, inner, HMAC_SHA256_DIGEST_SIZE);
    Crypto_sha256Final(&ctx, out);
    for (size_t i = 0; i < HMAC_SHA256_BLOCK_SIZE; ++i) {
        pad[i] = 0u;
        ipad[i] = 0u;
        opad[i] = 0u;
    }
    for (size_t i = 0; i < HMAC_SHA256_DIGEST_SIZE; ++i)
        inner[i] = 0u;
}
