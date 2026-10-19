#include "security/hkdf.h"

#include <stdlib.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "exception/throw.h"
#include "security/hmac.h"

;;DEFINITION
/** HKDF is two steps with one purpose: separation. Extract compresses whatever
 * entropy is available (a shared secret, a Diffie-Hellman result, a password
 * hash) into one fixed 32-byte pseudorandom key. Expand then stretches that key
 * into as many subkeys as a system needs, and each expansion is bound to a
 * caller-supplied `info` label, so the key used to encrypt is never the key used
 * to index or to sign. It adds no entropy — the input entropy is the ceiling —
 * but it removes the cross-protocol reuse that turns one leak into three.
 *
 * The `info` concatenation is why expand allocates a small cold buffer:
 * HMAC-SHA256 here is one-shot, and T(i-1) || info || counter has to be
 * contiguous. That allocation is per-call and wiped. Correctness is proved by
 * the RFC 5869 appendix A vectors; a wrong HKDF still returns plausible bytes. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Hkdf (security/hkdf.c)
 * FUNCTION REGISTRY (exports)
 *   Hkdf_extract(salt, saltLen, ikm, ikmLen, prk)
 *   Hkdf_expand(prk, info, infoLen, okm, okmLen)
 *   Hkdf(salt, saltLen, ikm, ikmLen, info, infoLen, okm, okmLen)
 * CONTRACT: expand refuses an output over 255*hash (one THROW) and a refused
 * work buffer; extract accepts a null/empty salt as 32 zero bytes. The PRK and
 * the work buffer are wiped. No logging beyond the cold rejection.
 * ======================================================================== */

void Hkdf_extract(const uint8_t *salt, size_t saltLen,
                  const uint8_t *ikm, size_t ikmLen,
                  uint8_t prk[HKDF_HASH_SIZE]) {
    static const uint8_t zeroSalt[HKDF_HASH_SIZE] = {0};
    if (prk == nullptr || (ikm == nullptr && ikmLen != 0u)) {
        THROW("Hkdf_extract: null output or null input with a nonzero length");
        return;
    }
    if (salt == nullptr || saltLen == 0u) {
        salt = zeroSalt;
        saltLen = HKDF_HASH_SIZE;
    }
    Hmac_sha256(salt, saltLen, ikm, ikmLen, prk);
}

void Hkdf_expand(const uint8_t prk[HKDF_HASH_SIZE], const uint8_t *info, size_t infoLen,
                 uint8_t *okm, size_t okmLen) {
    if (prk == nullptr || (okm == nullptr && okmLen != 0u) ||
        (info == nullptr && infoLen != 0u) || okmLen > HKDF_MAX_OUTPUT) {
        THROW("Hkdf_expand: null input or output length over the RFC ceiling");
        return;
    }
    if (okmLen == 0u)
        return;
    uint8_t t[HKDF_HASH_SIZE];
    size_t tLen = 0u;
    size_t offset = 0u;
    uint8_t counter = 1u;
    size_t bufferBytes = HKDF_HASH_SIZE + infoLen + 1u;
    uint8_t *buffer = (uint8_t*) malloc(bufferBytes);
    if (buffer == nullptr) {
        THROW("Hkdf_expand: work buffer of %zu bytes was refused", bufferBytes);
        return;
    }
    while (offset < okmLen) {
        size_t used = 0u;
        if (tLen > 0u) {
            memcpy(buffer, t, tLen);
            used += tLen;
        }
        if (infoLen > 0u) {
            memcpy(buffer + used, info, infoLen);
            used += infoLen;
        }
        buffer[used] = counter;
        ++used;
        Hmac_sha256(prk, HKDF_HASH_SIZE, buffer, used, t);
        tLen = HKDF_HASH_SIZE;
        size_t take = okmLen - offset < HKDF_HASH_SIZE ? okmLen - offset : HKDF_HASH_SIZE;
        memcpy(okm + offset, t, take);
        offset += take;
        ++counter;
    }
    memset(buffer, 0, bufferBytes);
    free(buffer);
    memset(t, 0, sizeof t);
}

void Hkdf(const uint8_t *salt, size_t saltLen, const uint8_t *ikm, size_t ikmLen,
          const uint8_t *info, size_t infoLen, uint8_t *okm, size_t okmLen) {
    uint8_t prk[HKDF_HASH_SIZE];
    Hkdf_extract(salt, saltLen, ikm, ikmLen, prk);
    Hkdf_expand(prk, info, infoLen, okm, okmLen);
    memset(prk, 0, sizeof prk);
}
