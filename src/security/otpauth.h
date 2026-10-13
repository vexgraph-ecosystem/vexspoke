#ifndef SECURITY_OTPAUTH_H
#define SECURITY_OTPAUTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/otpauth.h — the setup URI an authenticator app reads ONCE.
//
// otpauth:// is not an IETF RFC; it is the de-facto key-URI format every app
// implements (Google Authenticator, Authy, Apple Passwords, 1Password). Building
// it is the whole of "integrating" with them: there is no vendor API, no OAuth and
// nothing to poll. The URI is shown once at enrolment, usually as a QR code.
//
// Pure bytes → text: no allocation, no state. The label and issuer are
// percent-encoded per RFC 3986 (everything outside the unreserved set is escaped),
// so a space, a colon or an ampersand in a name cannot rewrite the query.

#define OTPAUTH_URI_RECOMMENDED 256u

// Writes `otpauth://totp/<issuer>:<account>?secret=<base32>&issuer=<issuer>
// &algorithm=SHA1&digits=<6..8>&period=<step>` into `dest` (NUL-terminated).
// False with *outTruncated set when `cap` cannot hold it, or on invalid input.
bool OtpAuth_buildUri(const char *issuer, const char *account,
                      const uint8_t *secret, size_t secretLen,
                      unsigned digits, unsigned stepSeconds,
                      char *dest, size_t cap, bool *outTruncated);

#endif
