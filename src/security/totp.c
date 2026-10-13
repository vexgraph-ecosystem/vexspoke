#include "security/totp.h"
#include "security/hmac.h"
#include "security/crypto.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/** TOTP: a shared per-user secret plus a counter derived from the clock. The code
 * is a truncated HMAC-SHA1 over that counter, which is why the digest module keeps
 * SHA-1 around. This file owns the two behaviours an authenticator app cannot
 * provide for you: a bounded acceptance window for clock skew, and replay refusal,
 * because a code that was valid once must not be valid twice. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Totp (security/totp.c)
 * FUNCTION REGISTRY (exports)
 *   Totp_code(secret, secretLen, unixTime, digits, stepSeconds)
 *   Totp_verify(secret, secretLen, code, unixTime, digits, stepSeconds, window, lastStep)
 * PRIVATE HELPERS:
 *   digitsModulus(digits)      the 10^digits modulus for 6..8, else 0
 *   stepCounter(step, into)    big-endian 8-byte counter
 * CONTRACT: a null secret or an out-of-range digit count rejects through THROW;
 * comparison uses Crypto_constantTimeEquals; no allocation, no logging on success.
 * ======================================================================== */

static uint32_t digitsModulus(unsigned digits) {
    switch (digits) {
        case 6u: return 1000000u;
        case 7u: return 10000000u;
        case 8u: return 100000000u;
        default: return 0u;
    }
}

static void stepCounter(uint64_t step, uint8_t out[8]) {
    for (unsigned i = 0; i < 8u; ++i)
        out[i] = (uint8_t) (step >> (56u - i * 8u));
}

uint32_t Totp_code(const uint8_t *secret, size_t secretLen, uint64_t unixTime,
                   unsigned digits, unsigned stepSeconds) {
    uint32_t modulus = digitsModulus(digits);
    if (secret == nullptr || secretLen == 0u || modulus == 0u || stepSeconds == 0u) {
        THROW("Totp_code: invalid secret, digits or step");
        return 0u;
    }
    uint8_t counter[8];
    stepCounter(unixTime / stepSeconds, counter);
    uint8_t mac[HMAC_SHA1_DIGEST_SIZE];
    Hmac_sha1(secret, secretLen, counter, sizeof counter, mac);
    unsigned offset = mac[HMAC_SHA1_DIGEST_SIZE - 1u] & 0x0Fu;
    uint32_t binary = ((uint32_t) (mac[offset] & 0x7Fu) << 24) |
                      ((uint32_t) mac[offset + 1u] << 16) |
                      ((uint32_t) mac[offset + 2u] << 8) |
                      (uint32_t) mac[offset + 3u];
    uint32_t code = binary % modulus;
    for (unsigned i = 0; i < sizeof mac; ++i)
        mac[i] = 0u;
    return code;
}

bool Totp_verify(const uint8_t *secret, size_t secretLen, uint32_t code, uint64_t unixTime,
                 unsigned digits, unsigned stepSeconds, int window, uint64_t *ioLastAcceptedStep) {
    if (secret == nullptr || secretLen == 0u || stepSeconds == 0u || digitsModulus(digits) == 0u) {
        THROW("Totp_verify: invalid secret, digits or step");
        return false;
    }
    int span = window < 0 ? 0 : window;
    uint64_t step = unixTime / stepSeconds;
    for (int offset = -span; offset <= span; ++offset) {
        if (offset < 0 && (uint64_t) (-offset) > step)
            continue;
        uint64_t candidate = offset < 0 ? step - (uint64_t) (-offset) : step + (uint64_t) offset;
        uint32_t expected = Totp_code(secret, secretLen, candidate * stepSeconds, digits, stepSeconds);
        if (!Crypto_constantTimeEquals(&expected, &code, sizeof expected))
            continue;
        if (ioLastAcceptedStep != nullptr) {
            if (candidate <= *ioLastAcceptedStep)
                return false;   // replay: this code was already accepted
            *ioLastAcceptedStep = candidate;
        }
        return true;
    }
    return false;
}
