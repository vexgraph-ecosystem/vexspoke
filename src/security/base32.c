#include "security/base32.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/** RFC 4648 base32, the encoding every authenticator app reads a TOTP secret in.
 * A pure format algorithm: borrowed bytes in, caller-owned bytes out, no file, no
 * device, no state, no allocation. It exists in `security/` because its one use is
 * the setup secret; it would move to a format module unchanged if a second user
 * appeared. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Base32 (security/base32.c)
 * FUNCTION REGISTRY (exports)
 *   Base32_encodedLength(byteLen) / Base32_decodedLength(charLen)
 *   Base32_encode(bytes, len, dest, cap, outTruncated)   dest-last, truncation-flagged
 *   Base32_decode(text, len, dest, cap, outLen)
 * PRIVATE HELPERS: decodeChar(c) — one alphabet character to 5 bits (-1 invalid)
 * CONTRACT: null input rejects through THROW; a too-small destination reports
 * truncation (encode) or rejects (decode); no allocation, no logging on success.
 * ======================================================================== */

static const char ALPHABET[] = BASE32_ALPHABET;

size_t Base32_encodedLength(size_t byteLen) { return ((byteLen + 4u) / 5u) * 8u; }
size_t Base32_decodedLength(size_t charLen) { return (charLen * 5u) / 8u; }

static int decodeChar(char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a';
    if (c >= '2' && c <= '7')
        return 26 + (c - '2');
    return -1;
}

bool Base32_encode(const uint8_t *bytes, size_t len, char *dest, size_t cap, bool *outTruncated) {
    if (outTruncated != nullptr)
        *outTruncated = false;
    if ((bytes == nullptr && len != 0u) || dest == nullptr) {
        THROW("Base32_encode: null input");
        return false;
    }
    size_t need = Base32_encodedLength(len);
    if (cap < need + 1u) {
        if (outTruncated != nullptr)
            *outTruncated = true;
        THROW("Base32_encode: destination too small");
        return false;
    }
    size_t out = 0;
    for (size_t i = 0; i < len; i += 5u) {
        size_t take = len - i < 5u ? len - i : 5u;
        uint64_t group = 0u;
        for (size_t j = 0; j < 5u; ++j)
            group = (group << 8) | (uint64_t) (j < take ? bytes[i + j] : 0u);
        for (unsigned j = 0; j < 8u; ++j)
            dest[out++] = ALPHABET[(group >> (35u - j * 5u)) & 0x1Fu];
        size_t pad = (5u - take) * 8u / 5u;
        for (size_t p = 0; p < pad; ++p)
            dest[out - 1u - p] = '=';
    }
    dest[out] = '\0';
    return true;
}

bool Base32_decode(const char *text, size_t len, uint8_t *dest, size_t cap, size_t *outLen) {
    if (outLen != nullptr)
        *outLen = 0u;
    if (text == nullptr || dest == nullptr) {
        THROW("Base32_decode: null input");
        return false;
    }
    size_t out = 0u;
    uint32_t acc = 0u;
    unsigned bits = 0u;
    for (size_t i = 0; i < len; ++i) {
        char c = text[i];
        if (c == '=')
            break;
        if (c == ' ' || c == '-' || c == '\n' || c == '\r' || c == '\t')
            continue;
        int value = decodeChar(c);
        if (value < 0) {
            THROW("Base32_decode: invalid character");
            return false;
        }
        acc = (acc << 5) | (uint32_t) value;
        bits += 5u;
        if (bits >= 8u) {
            bits -= 8u;
            if (out >= cap) {
                THROW("Base32_decode: destination too small");
                return false;
            }
            dest[out++] = (uint8_t) ((acc >> bits) & 0xFFu);
        }
    }
    if (outLen != nullptr)
        *outLen = out;
    return true;
}
