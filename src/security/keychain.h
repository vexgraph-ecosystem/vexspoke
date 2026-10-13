#ifndef SECURITY_KEYCHAIN_H
#define SECURITY_KEYCHAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/keychain.h — the OS credential store.
//
// One seam, three backends: macOS Keychain (Security.framework), Windows
// Credential Manager, Linux secret-service. It exists for material that must be
// READ BACK rather than merely verified — above all the encryption key for a TOTP
// secret, which is what lets a headless service start with no human present while
// still keeping that secret out of the database file.
//
// POLICY — the important line: where no keychain is available the call FAILS
// LOUDLY (KEYCHAIN_UNSUPPORTED). It must never silently fall back to a plaintext
// file. A security control that degrades quietly is worse than one that is absent,
// because the absence is at least visible.
//
// Secret bytes never appear in a log, a diagnostic or a projection.

#define KEYCHAIN_SERVICE_MAX 128u
#define KEYCHAIN_ACCOUNT_MAX 128u
#define KEYCHAIN_SECRET_MAX  512u

typedef enum KeychainCode {
    KEYCHAIN_OK = 0,
    KEYCHAIN_ABSENT = 1,       // no such item
    KEYCHAIN_DENIED = 2,       // the OS refused (user denied, ACL, locked)
    KEYCHAIN_UNSUPPORTED = 3,  // no backend on this host — never a silent fallback
    KEYCHAIN_INVALID = 4,      // null, empty or overlong argument
    KEYCHAIN_TOO_SMALL = 5,    // the item does not fit the caller's buffer
    KEYCHAIN_FULL = 6,         // the store refused for capacity/quota reasons
} KeychainCode;

// True when this host has a backend. False means every call returns UNSUPPORTED.
bool Keychain_isAvailable(void);
// Store or replace `len` bytes under (service, account). Replacing is idempotent.
KeychainCode Keychain_store(const char *service, const char *account,
                            const uint8_t *secret, size_t len);
// Read the item into `dest`; *outLen receives the exact byte count. TOO_SMALL when
// it does not fit — the caller may retry with a larger buffer, and `dest` is left
// untouched in that case.
KeychainCode Keychain_load(const char *service, const char *account,
                           uint8_t *dest, size_t cap, size_t *outLen);
KeychainCode Keychain_remove(const char *service, const char *account);
const char *Keychain_codeName(KeychainCode code);

#endif
