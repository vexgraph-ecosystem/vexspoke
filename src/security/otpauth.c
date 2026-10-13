#include "security/otpauth.h"
#include "security/base32.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include <stdio.h>

;;DEFINITION
/** The `otpauth://` setup URI: the one artifact that makes a darkbase account work
 * in an authenticator app. Because no vendor is involved, this file is the entire
 * integration surface — every app reads the same string. Names are percent-encoded
 * so a colon, space or ampersand in an issuer cannot silently rewrite the query
 * parameters, which is the kind of defect that would hand a scanned QR to the
 * wrong `secret=`. */
;;OVERVIEW
/** ========================================================================
 * MODULE: OtpAuth (security/otpauth.c)
 * FUNCTION REGISTRY (exports)
 *   OtpAuth_buildUri(issuer, account, secret, secretLen, digits, stepSeconds,
 *                    dest, cap, outTruncated)
 * PRIVATE HELPERS:
 *   isUnreserved(c)                    RFC 3986 unreserved set
 *   appendEncoded(dest, cap, used, text)  bounded percent-encoding append
 *   appendLiteral(dest, cap, used, text)  bounded literal append
 * CONTRACT: null names/secret, a null secret of nonzero length, digits outside
 * 6..8 or a zero step reject through THROW; a too-small buffer reports truncation.
 * ======================================================================== */

static bool isUnreserved(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '.' || c == '_' || c == '~';
}

static bool appendLiteral(char *dest, size_t cap, size_t *used, const char *text) {
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (*used + 2u > cap)
            return false;
        dest[(*used)++] = text[i];
    }
    dest[*used] = '\0';
    return true;
}

static bool appendEncoded(char *dest, size_t cap, size_t *used, const char *text) {
    static const char HEX[] = "0123456789ABCDEF";
    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (isUnreserved(c)) {
            if (*used + 2u > cap)
                return false;
            dest[(*used)++] = c;
        } else {
            if (*used + 4u > cap)
                return false;
            dest[(*used)++] = '%';
            dest[(*used)++] = HEX[(c >> 4) & 0x0Fu];
            dest[(*used)++] = HEX[c & 0x0Fu];
        }
    }
    dest[*used] = '\0';
    return true;
}

bool OtpAuth_buildUri(const char *issuer, const char *account,
                      const uint8_t *secret, size_t secretLen,
                      unsigned digits, unsigned stepSeconds,
                      char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated != nullptr)
        *outTruncated = false;
    if (issuer == nullptr || account == nullptr || secret == nullptr || secretLen == 0u ||
        dest == nullptr || digits < 6u || digits > 8u || stepSeconds == 0u) {
        THROW("OtpAuth_buildUri: invalid input");
        return false;
    }
    size_t used = 0u;
    char encoded[512];
    if (!Base32_encode(secret, secretLen, encoded, sizeof encoded, nullptr)) {
        THROW("OtpAuth_buildUri: secret too large to encode");
        return false;
    }
    char number[64];
    bool ok = appendLiteral(dest, cap, &used, "otpauth://totp/");
    if (ok)
        ok = appendEncoded(dest, cap, &used, issuer);
    if (ok)
        ok = appendLiteral(dest, cap, &used, ":");
    if (ok)
        ok = appendEncoded(dest, cap, &used, account);
    if (ok)
        ok = appendLiteral(dest, cap, &used, "?secret=");
    if (ok)
        ok = appendLiteral(dest, cap, &used, encoded);
    if (ok)
        ok = appendLiteral(dest, cap, &used, "&issuer=");
    if (ok)
        ok = appendEncoded(dest, cap, &used, issuer);
    if (ok) {
        // Checked, never cast away: a truncated query would silently drop the
        // digits or the period and hand the user a QR their app reads wrongly.
        int written = snprintf(number, sizeof number, "&algorithm=SHA1&digits=%u&period=%u",
                               digits, stepSeconds);
        if (written < 0 || (size_t) written >= sizeof number) {
            THROW("OtpAuth_buildUri: query buffer too small");
            return false;
        }
        ok = appendLiteral(dest, cap, &used, number);
    }
    if (!ok) {
        if (outTruncated != nullptr)
            *outTruncated = true;
        THROW("OtpAuth_buildUri: destination too small");
        return false;
    }
    return true;
}
