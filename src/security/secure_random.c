#include "security/secure_random.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: SecureRandom
 * ============================================================================
 * The OS-backed cryptographically secure random source. It holds no algorithmic
 * state of its own — every draw is delegated to the host's CSPRNG (Apple/BSD
 * arc4random_buf, Linux getrandom, Windows BCryptGenRandom, or /dev/urandom).
 * Delegating rather than rolling a DRBG is deliberate: the OS already implements
 * a seeded, reseeded, forward-secret CSPRNG, so there is nothing to get wrong.
 * A handle exists only to carry a draw counter (telemetry) and to give the
 * caller an explicit, non-seedable object. The struct is a cold heap handle.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: SecureRandom (security/secure_random.c)
 * LEVEL: L2 — Behavior (OS CSPRNG delegation)
 * ============================================================================
 * OS-backed cryptographically secure randomness. Non-reproducible by design;
 * it is the counterpart to util/random.h's seedable Random.
 *
 * STRUCT FIELDS (Mirroring security/secure_random.h):
 * ----------------------------------------------------------------------------
 *   SecureRandom {
 *     uint64_t draws; // count of draws issued (telemetry)
 *   }
 *
 * PRIVATE HELPERS:
 * ----------------------------------------------------------------------------
 *   osFill(dest, len)       : fill from the host CSPRNG; false when unavailable
 *   fillFromUrandom(dest, n): /dev/urandom fallback (POSIX)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - SecureRandom_0(void)
 *
 * Core Functions:
 *   - SecureRandom_free(self)
 *   - SecureRandom_bytes(self, dest, len)
 *   - SecureRandom_u64(self)
 *   - SecureRandom_u32(self)
 *
 * Getters:
 *   - SecureRandom_draws(self)
 * ============================================================================
 */

// --- OS CSPRNG backends -----------------------------------------------------
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__NetBSD__) || defined(__DragonFly__)
    #define SR_BACKEND_ARC4RANDOM 1
#elif defined(__linux__)
    #define SR_BACKEND_GETRANDOM 1
    #include <errno.h>
    #include <fcntl.h>
    #include <sys/random.h>
    #include <unistd.h>
#elif defined(_WIN32)
    #define SR_BACKEND_BCRYPT 1
    #include <windows.h>
    #include <bcrypt.h>
#else
    #define SR_BACKEND_URANDOM 1
    #include <fcntl.h>
    #include <unistd.h>
#endif

#if defined(SR_BACKEND_GETRANDOM) || defined(SR_BACKEND_URANDOM)
// POSIX fallback: bounded read loop over /dev/urandom.
static bool fillFromUrandom(void *dest, size_t len) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return false;
    uint8_t *p = (uint8_t*) dest;
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, p + got, len - got);
        if (n <= 0) {
            close(fd);
            return false;
        }
        got += (size_t) n;
    }
    close(fd);
    return true;
}
#endif

static bool osFill(void *dest, size_t len) {
    if (len == 0)
        return true;
    if (dest == nullptr)
        return false;

#if defined(SR_BACKEND_ARC4RANDOM)
    arc4random_buf(dest, len);
    return true;
#elif defined(SR_BACKEND_GETRANDOM)
    uint8_t *p = (uint8_t*) dest;
    size_t got = 0;
    while (got < len) {
        ssize_t n = getrandom(p + got, len - got, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return fillFromUrandom(p + got, len - got);
        }
        got += (size_t) n;
    }
    return true;
#elif defined(SR_BACKEND_BCRYPT)
    NTSTATUS status = BCryptGenRandom(nullptr, (PUCHAR) dest, (ULONG) len,
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return status == 0;
#else
    return fillFromUrandom(dest, len);
#endif
}

// --- Public API -------------------------------------------------------------
struct SecureRandom {
    uint64_t draws; // count of draws issued (telemetry)
};

SecureRandom *SecureRandom_0(void) {
    SecureRandom *self = (SecureRandom*) calloc(1, sizeof(SecureRandom));
    return self;
}

void SecureRandom_free(SecureRandom *self) {
    free(self);
}

bool SecureRandom_bytes(SecureRandom *self, void *dest, size_t len) {
    if (self == nullptr)
        return false;
    if (!osFill(dest, len))
        return false;
    (*self).draws++;
    return true;
}

uint64_t SecureRandom_u64(SecureRandom *self) {
    uint64_t value = 0;
    if (!SecureRandom_bytes(self, &value, sizeof value))
        return 0;
    return value;
}

uint32_t SecureRandom_u32(SecureRandom *self) {
    uint32_t value = 0;
    if (!SecureRandom_bytes(self, &value, sizeof value))
        return 0;
    return value;
}

uint64_t SecureRandom_draws(const SecureRandom *self) {
    return self ? (*self).draws : 0;
}
