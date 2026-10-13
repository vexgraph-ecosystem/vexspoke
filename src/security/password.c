#include "security/password.h"
#include "security/hmac.h"
#include "security/crypto.h"
#include "security/secure_random.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/** PBKDF2 as RFC 8018 defines it, over the HMACs this module already owns. Its
 * whole purpose is to be slow: the iteration count is the only thing standing
 * between a stolen file and a re-used password. It is therefore a cold path by
 * construction, and the default iteration count is a named default a caller may
 * raise — never a ceiling. Salt length is bounded so the derivation stays
 * zero-allocation; an over-long salt is refused rather than truncated, because a
 * silently shortened salt is a silently weaker hash. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Password (security/password.c)
 * FUNCTION REGISTRY (exports)
 *   Password_deriveSha1(password, salt, saltLen, iterations, outKey, keyLen)
 *   Password_deriveSha256(..same..)
 *   Password_verifySha256(password, salt, saltLen, iterations, expected, expectedLen)
 *   Password_generateSalt(dest, len)      OS CSPRNG
 * PRIVATE HELPERS:
 *   derive(prf, digestSize, password, salt, saltLen, iterations, outKey, keyLen)
 * CONTRACT: null/empty input, an out-of-range salt, zero iterations or a zero key
 * reject through THROW with the output untouched; no allocation; no logging.
 * ======================================================================== */

typedef void (*PasswordPrf)(const uint8_t *key, size_t keyLen, const uint8_t *data,
                            size_t dataLen, uint8_t *out);

static bool derive(PasswordPrf prf, size_t digestSize, const char *password, size_t passwordLen,
                   const uint8_t *salt, size_t saltLen, uint32_t iterations,
                   uint8_t *outKey, size_t keyLen) {
    if (prf == nullptr || password == nullptr || passwordLen == 0u || salt == nullptr ||
        outKey == nullptr || keyLen == 0u || iterations == 0u) {
        THROW("Password_derive: invalid input");
        return false;
    }
    if (saltLen == 0u || saltLen > PASSWORD_SALT_MAX) {
        // The ALGORITHM accepts any salt; the POLICY minimum applies when a salt is
        // generated (Password_generateSalt). Refusing a short salt here would make
        // PBKDF2 untestable against its own published vectors.
        THROW("Password_derive: salt length out of range");
        return false;
    }
    uint8_t message[PASSWORD_SALT_MAX + 4u];
    uint8_t u[32];
    uint8_t t[32];
    for (size_t i = 0; i < saltLen; ++i)
        message[i] = salt[i];
    size_t done = 0u;
    uint32_t block = 1u;
    while (done < keyLen) {
        message[saltLen]     = (uint8_t) (block >> 24);
        message[saltLen + 1] = (uint8_t) (block >> 16);
        message[saltLen + 2] = (uint8_t) (block >> 8);
        message[saltLen + 3] = (uint8_t) block;
        prf((const uint8_t*) password, passwordLen, message, saltLen + 4u, u);
        for (size_t i = 0; i < digestSize; ++i)
            t[i] = u[i];
        for (uint32_t round = 1u; round < iterations; ++round) {
            prf((const uint8_t*) password, passwordLen, u, digestSize, u);
            for (size_t i = 0; i < digestSize; ++i)
                t[i] ^= u[i];
        }
        size_t take = keyLen - done < digestSize ? keyLen - done : digestSize;
        for (size_t i = 0; i < take; ++i)
            outKey[done + i] = t[i];
        done += take;
        ++block;
    }
    for (size_t i = 0; i < sizeof message; ++i)
        message[i] = 0u;
    for (size_t i = 0; i < sizeof u; ++i) {
        u[i] = 0u;
        t[i] = 0u;
    }
    return true;
}

bool Password_deriveSha1(const char *password, const uint8_t *salt, size_t saltLen,
                         uint32_t iterations, uint8_t *outKey, size_t keyLen) {
    size_t len = password == nullptr ? 0u : __builtin_strlen(password);
    return derive(Hmac_sha1, HMAC_SHA1_DIGEST_SIZE, password, len, salt, saltLen, iterations, outKey, keyLen);
}

bool Password_deriveSha256(const char *password, const uint8_t *salt, size_t saltLen,
                           uint32_t iterations, uint8_t *outKey, size_t keyLen) {
    size_t len = password == nullptr ? 0u : __builtin_strlen(password);
    return derive(Hmac_sha256, HMAC_SHA256_DIGEST_SIZE, password, len, salt, saltLen, iterations, outKey, keyLen);
}

bool Password_verifySha256(const char *password, const uint8_t *salt, size_t saltLen,
                           uint32_t iterations, const uint8_t *expected, size_t expectedLen) {
    if (expected == nullptr || expectedLen == 0u || expectedLen > 32u) {
        THROW("Password_verifySha256: invalid stored key");
        return false;
    }
    uint8_t derived[32];
    if (!Password_deriveSha256(password, salt, saltLen, iterations, derived, expectedLen))
        return false;
    bool same = Crypto_constantTimeEquals(derived, expected, expectedLen);
    for (size_t i = 0; i < sizeof derived; ++i)
        derived[i] = 0u;
    return same;
}

bool Password_generateSalt(uint8_t *dest, size_t len) {
    if (dest == nullptr || len < PASSWORD_SALT_MIN || len > PASSWORD_SALT_MAX) {
        THROW("Password_generateSalt: invalid length (use the OS CSPRNG, not Crypto_prng*)");
        return false;
    }
    SecureRandom *rng = SecureRandom_0();
    if (rng == nullptr)
        return false;
    bool ok = SecureRandom_bytes(rng, dest, len);
    SecureRandom_free(rng);
    return ok;
}
