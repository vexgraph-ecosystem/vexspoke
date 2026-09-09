#include "annotation/intention.h"
#include "annotation/overview.h"
#include "system/process_probe.h"

#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <libproc.h>
#endif

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ProcessProbe (system/process_probe)
 * LEVEL: L2 — Behavior (live OS probes; no state, no allocation)
 * ============================================================================
 * The concurrency-free "what is running now" primitive: enumerates the
 * live process table via libproc and matches executable basenames
 * (case-insensitive), or scans a plugin directory for a loaded driver.
 * Pure C (darwin libproc + POSIX dirent) — no ObjC, no fork/exec, no
 * permission prompts: macOS allows other users' executable paths to be
 * read without screen-recording entitlements.
 *
 * STRUCT FIELDS (Mirroring system/process_probe.h — exactly this file's
 * class):
 * ----------------------------------------------------------------------------
 *   uint32_t reserved;  // singleton marker; no mutable state
 *
 * PRIVATE HELPERS (static pure functions, no state — documented here per
 * Rule 3 so the file reads from the overview alone):
 * ----------------------------------------------------------------------------
 *   processMatches(pid, wanted)  // pid_t + const char* — true when the
 *                                // pid's executable basename equals
 *                                // wanted (case-insensitive)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructor:
 *   - ProcessProbe_shared()
 * Core Functions:
 *   - ProcessProbe_isRunning(self, procName)
 *   - ProcessProbe_isDriverLoaded(self, dirPath, prefix)
 * ============================================================================
 */
;;INTENTION("darwin-only probe: non-Apple builds answer false — the engine is Apple Silicon native (Rule 13); a Linux procfs port is a later cycle")
;;INTENTION("basename compare is intentionally loose (no bundle-id verification): enough to answer 'is OBS alive'; false positives are documented per-row in the registries that consume this probe")

// Applies to macOS — darwin/Apple Silicon native engine (Rule 13).
#if defined(__APPLE__)

enum { kPidScanCap = 4096 }; // bounded stack scan; see PRIVATE HELPERS note

static bool processMatches(pid_t pid, const char *wanted) {
    char path[PROC_PIDPATHINFO_MAXSIZE];
    int len = proc_pidpath(pid, path, sizeof(path));
    if (len <= 0)
        return false;
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    return strcasecmp(base, wanted) == 0;
}

#endif

// CONSTRUCTORS

ProcessProbe *ProcessProbe_shared(void) {
    static ProcessProbe sProbeShared; // zero-init singleton
    return &sProbeShared;
}

// CORE FUNCTIONS

bool ProcessProbe_isRunning(const ProcessProbe *self, const char *procName) {
    if (!self || !procName || (*procName) == '\0')
        return false;
#if defined(__APPLE__)
    int maxPids = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    if (maxPids <= 0)
        return false;
    if (maxPids > kPidScanCap)
        maxPids = kPidScanCap;
    pid_t pids[kPidScanCap];
    int got = proc_listpids(PROC_ALL_PIDS, 0, pids, (int)sizeof(pids));
    if (got <= 0)
        return false;
    for (int i = 0; i < got && i < maxPids; i++) {
        if (pids[i] > 0 && processMatches(pids[i], procName))
            return true;
    }
    return false;
#else
    (void)procName;
    return false;
#endif
}

bool ProcessProbe_isDriverLoaded(const ProcessProbe *self,
                                 const char *dirPath, const char *prefix) {
    if (!self || !dirPath || !prefix || (*prefix) == '\0')
        return false;
    DIR *dir = opendir(dirPath);
    if (!dir)
        return false;
    const size_t prefixLen = strlen(prefix);
    struct dirent *entry;
    bool found = false;
    while ((entry = readdir(dir)) != NULL) {
        if (strncasecmp(entry->d_name, prefix, prefixLen) == 0) {
            found = true;
            break;
        }
    }
    closedir(dir);
    return found;
}