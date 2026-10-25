#include "security/keychain.h"
#include "exception/throw.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include <string.h>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#endif

;;DEFINITION
/** The OS credential store behind one seam. Only the Apple backend is built: the
 * Windows and secret-service backends are an explicit, documented gap rather than
 * a fake — this host cannot compile or prove them, and a backend that has never
 * run is not a backend. Where no backend exists the calls report UNSUPPORTED, so a
 * caller learns at the call site that the secret has nowhere safe to live; the one
 * thing this file will never do is quietly write the secret to disk instead. */
;;OVERVIEW
/** ========================================================================
 * MODULE: Keychain (security/keychain.c)
 * FUNCTION REGISTRY (exports)
 *   Keychain_isAvailable(void)
 *   Keychain_store(service, account, secret, len)
 *   Keychain_load(service, account, dest, cap, outLen)
 *   Keychain_remove(service, account)
 *   Keychain_codeName(code)
 * PRIVATE HELPERS (Apple):
 *   keysValid(service, account)        shared argument admission
 *   codeFor(OSStatus)                  OSStatus -> KeychainCode
 * CONTRACT: null/empty/overlong arguments are INVALID and never reach the OS;
 * store is idempotent (replace); load leaves `dest` untouched when it refuses;
 * no allocation of our own; secret bytes are never logged.
 * ======================================================================== */

const char *Keychain_codeName(KeychainCode code) {
    switch (code) {
        case KEYCHAIN_OK:          return "ok";
        case KEYCHAIN_ABSENT:      return "absent";
        case KEYCHAIN_DENIED:      return "denied";
        case KEYCHAIN_UNSUPPORTED: return "unsupported";
        case KEYCHAIN_INVALID:     return "invalid";
        case KEYCHAIN_TOO_SMALL:   return "too-small";
        case KEYCHAIN_FULL:        return "full";
        default:                   return "unknown";
    }
}

#if defined(__APPLE__)

static bool keysValid(const char *service, const char *account) {
    if (service == nullptr || account == nullptr)
        return false;
    size_t serviceLen = strlen(service);
    size_t accountLen = strlen(account);
    if (serviceLen == 0u || serviceLen >= KEYCHAIN_SERVICE_MAX)
        return false;
    if (accountLen == 0u || accountLen >= KEYCHAIN_ACCOUNT_MAX)
        return false;
    return true;
}

static KeychainCode codeFor(OSStatus status) {
    switch (status) {
        case errSecSuccess:        return KEYCHAIN_OK;
        case errSecItemNotFound:   return KEYCHAIN_ABSENT;
        case errSecAuthFailed:
        case errSecUserCanceled:
        case errSecInteractionNotAllowed: return KEYCHAIN_DENIED;
        case errSecAllocate:
        case errSecDiskFull:       return KEYCHAIN_FULL;
        default:                   return KEYCHAIN_DENIED;
    }
}

bool Keychain_isAvailable(void) { return true; }

KeychainCode Keychain_store(const char *service, const char *account,
                            const uint8_t *secret, size_t len) {
    if (!keysValid(service, account) || (secret == nullptr && len != 0u) || len > KEYCHAIN_SECRET_MAX) {
        THROW("Keychain_store: invalid argument");
        return KEYCHAIN_INVALID;
    }
    CFStringRef svc = CFStringCreateWithCString(nullptr, service, kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithCString(nullptr, account, kCFStringEncodingUTF8);
    CFDataRef data = CFDataCreate(nullptr, secret, (CFIndex) len);
    if (svc == nullptr || acct == nullptr || data == nullptr) {
        if (svc != nullptr) CFRelease(svc);
        if (acct != nullptr) CFRelease(acct);
        if (data != nullptr) CFRelease(data);
        THROW("Keychain_store: could not build the query");
        return KEYCHAIN_FULL;
    }
    const void *keys[] = { kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData };
    const void *values[] = { kSecClassGenericPassword, svc, acct, data };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 4,
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);
    KeychainCode result = KEYCHAIN_FULL;
    if (query != nullptr) {
        // Idempotent: replace any existing item rather than failing as a duplicate.
        SecItemDelete(query);
        OSStatus status = SecItemAdd(query, nullptr);
        result = codeFor(status);
        if (result != KEYCHAIN_OK)
            THROW("Keychain_store: the OS refused the item");
        CFRelease(query);
    }
    CFRelease(svc);
    CFRelease(acct);
    CFRelease(data);
    return result;
}

KeychainCode Keychain_load(const char *service, const char *account,
                           uint8_t *dest, size_t cap, size_t *outLen) {
    if (outLen != nullptr)
        *outLen = 0u;
    if (!keysValid(service, account) || dest == nullptr || cap == 0u) {
        THROW("Keychain_load: invalid argument");
        return KEYCHAIN_INVALID;
    }
    CFStringRef svc = CFStringCreateWithCString(nullptr, service, kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithCString(nullptr, account, kCFStringEncodingUTF8);
    if (svc == nullptr || acct == nullptr) {
        if (svc != nullptr) CFRelease(svc);
        if (acct != nullptr) CFRelease(acct);
        return KEYCHAIN_FULL;
    }
    const void *keys[] = { kSecClass, kSecAttrService, kSecAttrAccount, kSecReturnData, kSecMatchLimit };
    const void *values[] = { kSecClassGenericPassword, svc, acct, kCFBooleanTrue, kSecMatchLimitOne };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 5,
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);
    KeychainCode result = KEYCHAIN_FULL;
    if (query != nullptr) {
        CFTypeRef item = nullptr;
        OSStatus status = SecItemCopyMatching(query, &item);
        result = codeFor(status);
        if (result == KEYCHAIN_OK && item != nullptr) {
            size_t length = (size_t) CFDataGetLength((CFDataRef) item);
            if (length > cap) {
                result = KEYCHAIN_TOO_SMALL;   // dest untouched
            } else {
                memcpy(dest, CFDataGetBytePtr((CFDataRef) item), length);
                if (outLen != nullptr)
                    *outLen = length;
            }
        }
        if (item != nullptr)
            CFRelease(item);
        CFRelease(query);
    }
    CFRelease(svc);
    CFRelease(acct);
    return result;
}

KeychainCode Keychain_remove(const char *service, const char *account) {
    if (!keysValid(service, account)) {
        THROW("Keychain_remove: invalid argument");
        return KEYCHAIN_INVALID;
    }
    CFStringRef svc = CFStringCreateWithCString(nullptr, service, kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithCString(nullptr, account, kCFStringEncodingUTF8);
    if (svc == nullptr || acct == nullptr) {
        if (svc != nullptr) CFRelease(svc);
        if (acct != nullptr) CFRelease(acct);
        return KEYCHAIN_FULL;
    }
    const void *keys[] = { kSecClass, kSecAttrService, kSecAttrAccount };
    const void *values[] = { kSecClassGenericPassword, svc, acct };
    CFDictionaryRef query = CFDictionaryCreate(nullptr, keys, values, 3,
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);
    KeychainCode result = KEYCHAIN_FULL;
    if (query != nullptr) {
        result = codeFor(SecItemDelete(query));
        CFRelease(query);
    }
    CFRelease(svc);
    CFRelease(acct);
    return result;
}

#else

// No backend on this host, and no fallback: the caller must know the secret has
// nowhere safe to live rather than discover a plaintext copy later.
bool Keychain_isAvailable(void) { return false; }

KeychainCode Keychain_store(const char *service, const char *account,
                            const uint8_t *secret, size_t len) {
    (void) service; (void) account; (void) secret; (void) len;
    THROW("Keychain_store: no keychain backend on this host");
    return KEYCHAIN_UNSUPPORTED;
}

KeychainCode Keychain_load(const char *service, const char *account,
                           uint8_t *dest, size_t cap, size_t *outLen) {
    (void) service; (void) account; (void) dest; (void) cap;
    if (outLen != nullptr)
        *outLen = 0u;
    THROW("Keychain_load: no keychain backend on this host");
    return KEYCHAIN_UNSUPPORTED;
}

KeychainCode Keychain_remove(const char *service, const char *account) {
    (void) service; (void) account;
    THROW("Keychain_remove: no keychain backend on this host");
    return KEYCHAIN_UNSUPPORTED;
}

#endif
