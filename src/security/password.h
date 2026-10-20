#ifndef SECURITY_PASSWORD_H
#define SECURITY_PASSWORD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// security/password.h — versioned slow password verifiers (PBKDF2 or Argon2id).
//
// A password is NEVER stored. What is stored is a salted, deliberately slow
// derivation, so that a stolen file costs an attacker real time per guess.
//
// This is a COLD path and only ever a cold path: deriving a hash burns
// iterations on purpose. It must never be reachable from a hot path, and it must
// never be called per request — per-request verification checks a session token
// (the Session Token Law in darkbase-headless), never a password.
//
// The raw PBKDF2 path is zero-allocation: the salt is bound to a fixed maximum, and a
// longer salt rejects loudly rather than being silently truncated. PASSWORD_SALT_MIN is
// a POLICY bound enforced by Password_generateSalt, not by the derivation: the algorithm
// accepts any salt (that is what lets it be checked against its published RFC vectors).

#define PASSWORD_SALT_MIN         16u
#define PASSWORD_SALT_MAX         64u
#define PASSWORD_KEY_DEFAULT       32u
#define PASSWORD_ITERATIONS_MIN    1000u
// A named default, not a ceiling: callers raise it as hardware allows.
#define PASSWORD_ITERATIONS_DEFAULT 210000u

typedef enum PasswordKdf {
    PASSWORD_KDF_PBKDF2_SHA256 = 1u,
    PASSWORD_KDF_ARGON2ID = 2u
} PasswordKdf;

typedef enum PasswordCode {
    PASSWORD_CODE_OK = 0,
    PASSWORD_CODE_INVALID = 1,
    PASSWORD_CODE_UNSUPPORTED_KDF = 2,
    PASSWORD_CODE_DERIVATION_FAILED = 3
} PasswordCode;

typedef struct PasswordVerifier {
    uint32_t code;
    uint32_t kdf;
    uint32_t iterations;
    uint32_t memoryKiB;
    uint32_t lanes;
    uint8_t salt[PASSWORD_SALT_MAX];
    uint32_t saltLength;
    uint8_t hash[32];
    uint32_t hashLength;
} PasswordVerifier;

// Raw derivation. `prfDigest` selects the MAC width (20 for SHA-1, 32 for SHA-256).
bool Password_deriveSha1(const char *password, const uint8_t *salt, size_t saltLen,
                         uint32_t iterations, uint8_t *outKey, size_t keyLen);
bool Password_deriveSha256(const char *password, const uint8_t *salt, size_t saltLen,
                           uint32_t iterations, uint8_t *outKey, size_t keyLen);
// Constant-time comparison against a stored key.
bool Password_verifySha256(const char *password, const uint8_t *salt, size_t saltLen,
                           uint32_t iterations, const uint8_t *expected, size_t expectedLen);
// A fresh salt from the OS CSPRNG (never from the deterministic Crypto_prng*).
bool Password_generateSalt(uint8_t *dest, size_t len);

// Versioned verifier record. Creation generates a fresh OS-backed salt and
// dispatches to the selected KDF; verification dispatches only on the stored
// version and rejects malformed parameters without changing the record.
PasswordVerifier Password_createVerifier(PasswordKdf kdf, const char *password);
bool PasswordVerifier_isValid(const PasswordVerifier *self);
bool PasswordVerifier_verify(const PasswordVerifier *self, const char *password);

#endif
