#ifndef SPOKE_BESPOKE_H
#define SPOKE_BESPOKE_H

#include <stdbool.h>
#include <stdint.h>

// spoke/bespoke.h — R2 Behavior: Unified Bespoke Bridge Hub.
//
// Single Class Per File Law: Bespoke.
// Vertical Integration Law (Supervisor Order R1 > R2 > R3 > R4 > R5):
// vexspoke is a pure leaf. It declares weak extern hooks that downstream
// R3 drivers (graphvex for graphics, api-haven for networking) implement.
// At boot, bridgeBespoke() invokes linked drivers; bridgeBespokeCheck()
// services late-starting drivers and restarts hot-replaced ones.

typedef enum BespokeState {
    BESPOKE_ABSENT = 0,    // Driver not linked or disabled
    BESPOKE_LATE = 1,      // Driver linked, but waiting for prerequisites (window, device, etc.)
    BESPOKE_READY = 2,     // Ready to run
    BESPOKE_RUNNING = 3,   // Actively running
    BESPOKE_REPLACED = 4,  // Driver was hot-swapped/reloaded; needs re-running
    BESPOKE_STOPPED = 5    // Cleanly finished / stopped
} BespokeState;

typedef struct Bespoke {
    BespokeState graphicsState;
    BespokeState networkingState;
    BespokeState smthState;
    uint64_t runs;
    uint64_t checks;
} Bespoke;

// --- Weak extern driver hooks (R3 graphvex / api-haven implementations) ---
#if defined(__APPLE__)
#  define BESPOKE_WEAK __attribute__((weak_import))
#elif defined(__linux__)
#  define BESPOKE_WEAK __attribute__((weak))
#else
#  define BESPOKE_WEAK
#endif

extern bool         runGraphics(void)   BESPOKE_WEAK;
extern BespokeState checkGraphics(void) BESPOKE_WEAK;

extern bool         runNetworking(void)   BESPOKE_WEAK;
extern BespokeState checkNetworking(void) BESPOKE_WEAK;

extern bool         doSmth(void)        BESPOKE_WEAK;
extern BespokeState checkSmth(void)     BESPOKE_WEAK;

// --- Public Hub Functions ---
void         bridgeBespoke(void);
bool         bridgeBespokeCheck(void);
BespokeState bridgeBespokeGraphicsState(void);
BespokeState bridgeBespokeNetworkingState(void);
BespokeState bridgeBespokeSmthState(void);

// --- Class Functions ---
Bespoke     *Bespoke_default(void);
void         Bespoke_bridge(Bespoke *self);
bool         Bespoke_check(Bespoke *self);

#endif
