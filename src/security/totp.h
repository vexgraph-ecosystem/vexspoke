#ifndef SECURITY_TOTP_H
#define SECURITY_TOTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/totp.h — TOTP (RFC 6238 over RFC 4226): the second factor.
//
// There is no vendor here. Every authenticator app — Google Authenticator, Authy,
// Apple Passwords, 1Password — implements these two RFCs and reads the
// `otpauth://` setup URI, which is why one enrolment works everywhere. The interop
// surface is the RFC and nothing else.
//
// Verification takes a bounded window for clock skew and REFUSES REPLAY: the
// caller passes the last accepted step per user, and a code at or before it is
// rejected for the rest of its validity. Without that, one 30-second code with
// tolerance stays reusable for ~90 seconds by anyone who saw it.

#define TOTP_STEP_SECONDS_DEFAULT 30u
#define TOTP_DIGITS_DEFAULT       6u
#define TOTP_DIGITS_MAX           8u
#define TOTP_WINDOW_DEFAULT       1   // ±1 step, the usual clock-skew allowance

// The code for one instant. `digits` is 6..8; anything else is rejected (returns 0).
uint32_t Totp_code(const uint8_t *secret, size_t secretLen, uint64_t unixTime,
                   unsigned digits, unsigned stepSeconds);

// True when `code` is valid for `unixTime` within ±`window` steps. When
// `ioLastAcceptedStep` is non-null it is read as the last accepted step and
// written back on success, so a replayed code is refused. Null means "no replay
// tracking" — the caller takes responsibility for it.
bool Totp_verify(const uint8_t *secret, size_t secretLen, uint32_t code, uint64_t unixTime,
                 unsigned digits, unsigned stepSeconds, int window, uint64_t *ioLastAcceptedStep);

#endif
