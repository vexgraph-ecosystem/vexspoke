#include "annotation/intention.h"
#include "annotation/overview.h"
#include "system/capture_tool.h"
#include "system/process_probe.h"

#include <string.h>

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: CaptureTool (system/capture_tool)
 * LEVEL: L1 — File Metadata over L2 probes (declarative rows + live
 * process/driver liveness; extendable with zero code changes)
 * ============================================================================
 * The capture/recording directory: 15 static descriptor rows over the
 * capture-capable tools of the platform (OBS, QuickTime Player, the
 * macOS Screenshot utility, Screen Studio, CleanShot X, Kap, Loom,
 * ScreenFlow, Camtasia, Filmage, Zoom, Voice Memos, Loopback, plus the
 * BlackHole / Soundflower CoreAudio HAL virtual-audio drivers). Each row
 * is probed live: process rows match the executable basename, driver
 * rows scan the plugin directory. No fork/exec, no permission prompts,
 * no allocation.
 *
 * STRUCT FIELDS (Mirroring system/capture_tool.h — exactly this file's
 * class):
 * ----------------------------------------------------------------------------
 *   uint32_t reserved;  // singleton marker; no mutable state — all data
 *                       // lives in the static const CaptureSlot rows
 *
 * SLOT RECORD (CaptureSlot — Rule 3 co-location, zero behavior of its own):
 * ----------------------------------------------------------------------------
 *   const char *slug;         // canonical key, e.g. "obs"
 *   const char *displayName;  // human label, e.g. "OBS Studio"
 *   const char *procKey;      // executable basename; NULL = driver row
 *   const char *driverDir;    // plugin dir scanned; NULL = process row
 *   CaptureKind kind;         // CAPTURE_KIND_SCREEN / STREAM / AUDIO
 *   const char *note;         // caveat; NULL when none
 *
 * PRIVATE HELPERS (data only, per-field roles = the SLOT RECORD above):
 * ----------------------------------------------------------------------------
 *   kCaptureTools[] — src/system/data/capture_tools.inc (15 hand-curated
 *                     rows, included ONLY by capture_tool.c)
 *   kCaptureToolCount — row count constant
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructor:
 *   - CaptureTool_shared() : singleton directory handle
 *
 * Core Functions:
 *   - CaptureTool_count / at / get(self, ...) : registry access
 *   - CaptureTool_isInstalled(self, slot)     : process alive OR driver loaded
 *   - CaptureTool_isRunning(self, slot)       : process alive now (proxy for
 *                                               "recording right now"; false
 *                                               for driver rows)
 *   - CaptureTool_countRunning(self, kind)    : live count per kind
 *   - CaptureTool_countRunningAll(self)       : live count, any kind
 *
 * Getters (Rule 24; null-safe. Setters omitted — immutable rows, waiver):
 *   - CaptureTool_getSlug / getDisplayName / getProcKey / getDriverDir /
 *     getKind / getNote(self, slot)
 * ============================================================================
 */
;;INTENTION("macOS hides other apps' active ScreenCaptureKit sessions by design — no public API reveals 'the system is recording'. This class honestly reports tool liveness (OBS alive => could be capturing); a true privacy-dot signal would need private API and is out of scope")
;;INTENTION("drivers (BlackHole/Soundflower) have no process — liveness answers false by contract; isInstalled probes the HAL plugin dir instead")
;;INTENTION("proc basenames are hand-curated and version-sensitive (Camtasia 2023); a miss degrades to not-running, never to a wrong 'recording' claim — single-process rows keep false positives at zero")

#include "system/data/capture_tools.inc"

static const uint32_t kCaptureToolCount =
    (uint32_t)(sizeof(kCaptureTools) / sizeof(kCaptureTools[0]));

// CONSTRUCTORS

CaptureTool *CaptureTool_shared(void) {
    static CaptureTool sCaptureShared; // zero-init singleton
    return &sCaptureShared;
}

// CORE FUNCTIONS

uint32_t CaptureTool_count(const CaptureTool *self) {
    if (!self)
        return 0;
    return kCaptureToolCount;
}

const CaptureSlot *CaptureTool_at(const CaptureTool *self, uint32_t i) {
    if (!self || i >= kCaptureToolCount)
        return NULL;
    return &kCaptureTools[i];
}

const CaptureSlot *CaptureTool_get(const CaptureTool *self, const char *slug) {
    if (!self || !slug || (*slug) == '\0')
        return NULL;
    for (uint32_t i = 0; i < kCaptureToolCount; i++) {
        if (kCaptureTools[i].slug && strcmp(kCaptureTools[i].slug, slug) == 0)
            return &kCaptureTools[i];
    }
    return NULL;
}

bool CaptureTool_isInstalled(const CaptureTool *self, const CaptureSlot *slot) {
    if (!self || !slot)
        return false;
    const char *procKey = (*slot).procKey;
    if (procKey && (*procKey) != '\0') {
        ProcessProbe *probe = ProcessProbe_shared();
        return ProcessProbe_isRunning(probe, procKey);
    }
    const char *driverDir = (*slot).driverDir;
    if (driverDir && (*driverDir) != '\0') {
        ProcessProbe *probe = ProcessProbe_shared();
        return ProcessProbe_isDriverLoaded(probe, driverDir, (*slot).slug);
    }
    return false;
}

bool CaptureTool_isRunning(const CaptureTool *self, const CaptureSlot *slot) {
    if (!self || !slot)
        return false;
    const char *procKey = (*slot).procKey;
    if (!procKey || (*procKey) == '\0')
        return false; // driver rows have no process — liveness is N/A
    ProcessProbe *probe = ProcessProbe_shared();
    return ProcessProbe_isRunning(probe, procKey);
}

uint32_t CaptureTool_countRunning(const CaptureTool *self, CaptureKind kind) {
    if (!self)
        return 0;
    uint32_t running = 0;
    for (uint32_t i = 0; i < kCaptureToolCount; i++) {
        if (kCaptureTools[i].kind == kind && CaptureTool_isRunning(self, &kCaptureTools[i]))
            running++;
    }
    return running;
}

uint32_t CaptureTool_countRunningAll(const CaptureTool *self) {
    if (!self)
        return 0;
    uint32_t running = 0;
    for (uint32_t i = 0; i < kCaptureToolCount; i++) {
        if (CaptureTool_isRunning(self, &kCaptureTools[i]))
            running++;
    }
    return running;
}

// GETTERS

const char *CaptureTool_getSlug(const CaptureTool *self, const CaptureSlot *slot) {
    (void)self;
    return slot ? (*slot).slug : NULL;
}

const char *CaptureTool_getDisplayName(const CaptureTool *self, const CaptureSlot *slot) {
    (void)self;
    return slot ? (*slot).displayName : NULL;
}

const char *CaptureTool_getProcKey(const CaptureTool *self, const CaptureSlot *slot) {
    (void)self;
    return slot ? (*slot).procKey : NULL;
}

const char *CaptureTool_getDriverDir(const CaptureTool *self, const CaptureSlot *slot) {
    (void)self;
    return slot ? (*slot).driverDir : NULL;
}

CaptureKind CaptureTool_getKind(const CaptureTool *self, const CaptureSlot *slot) {
    if (!self || !slot)
        return CAPTURE_KIND_SCREEN; // safe default
    return (*slot).kind;
}

const char *CaptureTool_getNote(const CaptureTool *self, const CaptureSlot *slot) {
    (void)self;
    return slot ? (*slot).note : NULL;
}