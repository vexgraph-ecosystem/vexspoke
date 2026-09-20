#include "spoke/bespoke.h"

#include <stddef.h>
#include "annotation/overview.h"
#include "annotation/intention.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Bespoke (spoke/bespoke.c)
 * LEVEL: L2 — Behavior (unified bespoke bridge hub)
 * ============================================================================
 * Unified Bespoke Bridge Hub. Coordinates R3 drivers (graphics, networking,
 * storage/etc.) via weak extern function hooks. Supports initial launch
 * (bridgeBespoke) and periodic re-check (bridgeBespokeCheck) to service
 * late-starting drivers and restart hot-replaced ones.
 *
 * STRUCT FIELDS (Mirroring spoke/bespoke.h — exactly this file's class):
 * ----------------------------------------------------------------------------
 *   BespokeState graphicsState;
 *   BespokeState networkingState;
 *   BespokeState smthState;
 *   uint64_t runs;
 *   uint64_t checks;
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Bespoke_default()
 * Core Functions:
 *   - bridgeBespoke()
 *   - bridgeBespokeCheck()
 *   - Bespoke_bridge(self)
 *   - Bespoke_check(self)
 * Getters:
 *   - bridgeBespokeGraphicsState()
 *   - bridgeBespokeNetworkingState()
 *   - bridgeBespokeSmthState()
 * ============================================================================
 */

#if defined(__APPLE__) || defined(__linux__)
__attribute__((weak)) bool runGraphics(void) { return false; }
__attribute__((weak)) BespokeState checkGraphics(void) { return BESPOKE_ABSENT; }
__attribute__((weak)) bool runNetworking(void) { return false; }
__attribute__((weak)) BespokeState checkNetworking(void) { return BESPOKE_ABSENT; }
__attribute__((weak)) bool doSmth(void) { return false; } // this is just a placeholder
__attribute__((weak)) BespokeState checkSmth(void) { return BESPOKE_ABSENT; }
#endif

static Bespoke s_defaultBespoke = {
    .graphicsState = BESPOKE_ABSENT,
    .networkingState = BESPOKE_ABSENT,
    .smthState = BESPOKE_ABSENT,
    .runs = 0,
    .checks = 0
};

Bespoke *Bespoke_default(void) {
    return &s_defaultBespoke;
}

void Bespoke_bridge(Bespoke *self) {
    if (!self)
        return;

    (*self).runs++;

    // 1. Networking (api-haven)
    if (runNetworking != nullptr) {
        BespokeState state = checkNetworking ? checkNetworking() : BESPOKE_READY;
        if (state == BESPOKE_READY) {
            if (runNetworking())
                (*self).networkingState = BESPOKE_RUNNING;
            else
                (*self).networkingState = BESPOKE_LATE;
        } else {
            (*self).networkingState = state;
        }
    } else {
        (*self).networkingState = BESPOKE_ABSENT;
    }

    // 2. Custom / doSmth (darkbase or other)
    if (doSmth != nullptr) {
        BespokeState state = checkSmth ? checkSmth() : BESPOKE_READY;
        if (state == BESPOKE_READY) {
            if (doSmth())
                (*self).smthState = BESPOKE_RUNNING;
            else
                (*self).smthState = BESPOKE_LATE;
        } else {
            (*self).smthState = state;
        }
    } else {
        (*self).smthState = BESPOKE_ABSENT;
    }

    // 3. Graphics (graphvex)
    if (runGraphics != nullptr) {
        BespokeState state = checkGraphics ? checkGraphics() : BESPOKE_READY;
        if (state == BESPOKE_READY) {
            if (runGraphics())
                (*self).graphicsState = BESPOKE_RUNNING;
            else
                (*self).graphicsState = BESPOKE_LATE;
        } else {
            (*self).graphicsState = state;
        }
    } else {
        (*self).graphicsState = BESPOKE_ABSENT;
    }
}

bool Bespoke_check(Bespoke *self) {
    if (!self)
        return false;

    (*self).checks++;
    bool changed = false;

    // 1. Check Graphics: handle late arrival or hot replacement
    if (runGraphics != nullptr) {
        BespokeState live = checkGraphics ? checkGraphics() : (*self).graphicsState;
        if (live == BESPOKE_READY || live == BESPOKE_REPLACED) {
            if (runGraphics()) {
                (*self).graphicsState = BESPOKE_RUNNING;
                changed = true;
            }
        } else if (live != (*self).graphicsState) {
            (*self).graphicsState = live;
            changed = true;
        }
    }

    // 2. Check Networking
    if (runNetworking != nullptr) {
        BespokeState live = checkNetworking ? checkNetworking() : (*self).networkingState;
        if (live == BESPOKE_READY || live == BESPOKE_REPLACED) {
            if (runNetworking()) {
                (*self).networkingState = BESPOKE_RUNNING;
                changed = true;
            }
        } else if (live != (*self).networkingState) {
            (*self).networkingState = live;
            changed = true;
        }
    }

    // 3. Check doSmth
    if (doSmth != nullptr) {
        BespokeState live = checkSmth ? checkSmth() : (*self).smthState;
        if (live == BESPOKE_READY || live == BESPOKE_REPLACED) {
            if (doSmth()) {
                (*self).smthState = BESPOKE_RUNNING;
                changed = true;
            }
        } else if (live != (*self).smthState) {
            (*self).smthState = live;
            changed = true;
        }
    }

    return changed;
}

void bridgeBespoke(void) {
    Bespoke_bridge(&s_defaultBespoke);
}

bool bridgeBespokeCheck(void) {
    return Bespoke_check(&s_defaultBespoke);
}

BespokeState bridgeBespokeGraphicsState(void) {
    return s_defaultBespoke.graphicsState;
}

BespokeState bridgeBespokeNetworkingState(void) {
    return s_defaultBespoke.networkingState;
}

BespokeState bridgeBespokeSmthState(void) {
    return s_defaultBespoke.smthState;
}
