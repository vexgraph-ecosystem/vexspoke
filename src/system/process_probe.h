#ifndef SYSTEM_PROCESS_PROBE_H
#define SYSTEM_PROCESS_PROBE_H

#include <stdbool.h>
#include <stdint.h>

// system/process_probe.h — live process & driver probing (darwin).
//
// Answers "is that process actually running RIGHT NOW" and "is that
// low-level driver/hook installed" without fork/exec — the low-level
// capability probe underneath AppDetect (isRunning) and CaptureTool
// (recording-tool liveness).
//
// Implementation: libproc pid enumeration + proc_pidpath basename
// compare (case-insensitive, no permission required to read paths of
// other users' processes on macOS). Driver probe: opendir prefix scan
// under a given plugin directory (e.g. /Library/Audio/Plug-Ins/HAL).
// Non-macOS platforms answer false; see ;;INTENTION in process_probe.c.

typedef struct ProcessProbe {
    uint32_t reserved; // signature; no mutable state
} ProcessProbe;

// --- Constructor ---
// Returns the shared probe handle (static, zero-init, never NULL).
ProcessProbe *ProcessProbe_shared(void);

// --- Core functions ---
// true when any process's executable basename equals procName
// (case-insensitive, e.g. "obs", "QuickTime Player"). False on NULL
// args, empty names, or when the platform cannot enumerate.
bool ProcessProbe_isRunning(const ProcessProbe *self, const char *procName);
// true when dirPath exists and contains an entry whose name starts with
// prefix (case-insensitive) — e.g. ("/Library/Audio/Plug-Ins/HAL",
// "blackhole") matches "BlackHole2ch.driver". False on NULL args or a
// missing/unreadable directory.
bool ProcessProbe_isDriverLoaded(const ProcessProbe *self,
                                 const char *dirPath, const char *prefix);

#endif