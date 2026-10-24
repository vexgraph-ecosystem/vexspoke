#include "system/system.h"

#include "cli/console.h"
#include "input/key.h"
#include "input/mouse.h"
#include "input/touch.h"
#include "time/nanotime.h"
#include "io/hot_file.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

#include <stdlib.h>
#include <string.h>

extern char **environ;  // the process environment table (libc-owned)

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: System
 * ============================================================================
 * Unified native system information, discovery, and display subsystem boot
 * entry. System_initializeAll is the init-once boot seam that starts the
 * engine subsystems (NanoTime, Console, Key, Mouse, Touch, HotFile watcher)
 * in dependency order; the VFS boots later in darling once paths resolve.
 * Procedural — owns no struct; all state lives in the subsystems it boots.
 *
 * System also owns the process-environment surface (System_getEnvironment /
 * System_getEnvValue). One class owns envs: the environment is explicit session
 * context, captured once and read by name, never an ambient getenv at the call
 * site. See the INTENTIONAL(vex) note below.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: System (system/system.c)
 * ============================================================================
 * Unified native system information, discovery, and display subsystem
 *
 * STRUCT FIELDS: none — procedural (operates on engine subsystems (init-once boot))
 *
 * MODULE STATE (file-local, the captured environment):
 * ----------------------------------------------------------------------------
 *   sEntries    // owned "NAME=VALUE" snapshot of the environment
 *   sEntryCount // number of entries in sEntries
 *   sDebugPath  // owned debug / current-workflow source path
 *   sExportPath // owned manifest-derived export destination path
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - System_initializeAll(void)
 * Environment:
 *   - System_getEnvironment(debugpath, exportpath) : capture + bind both paths
 *   - System_getEnvValue(name)                      : borrowed value or null
 *   - System_hasEnv(name)                           : presence test
 *   - System_freeEnvironment(void)                  : release the capture (idempotent)
 * Private helpers (file-local):
 *   - duplicateText(text) : owned copy or null
 *   - releaseEnvState(void) : free the capture and reset to empty
 * ============================================================================
 */


void System_initializeAll(void) {
    NanoTime_init();
    Console_init();
    Key_init();
    Mouse_init();
    Touch_init();

    // NOTE: Vfs_init() lives in darling (io/vfs.c) since the split — the
    // UI/bake layer boots the VFS itself once paths are resolvable there.
    // Boot the HotFile watcher
    HotFileSys_init();
}

// INTENTIONAL(vex): the process environment is owned by System, not a separate
// Env class. One class handles envs. System_getEnvironment binds the debug source
// and the manifest-derived export destination up front and captures the values a
// run sees once; System_getEnvValue reads that snapshot by name. Nothing is
// resolved from the CWD or a hidden default — env access is a function of the
// arguments, not of the launch environment. This shape is deliberate.
;;INTENTION("environment is explicit session context, not an ambient global; both paths are caller-supplied strings per the Conflict Triage Law managed-argument rule")

static char **sEntries = nullptr;     // owned "NAME=VALUE" snapshot
static size_t sEntryCount = 0;        // entries in sEntries
static char *sDebugPath = nullptr;    // owned debug / current-workflow path
static char *sExportPath = nullptr;   // owned manifest-derived export path

static char *duplicateText(const char *text) {
    if (text == nullptr)
        return nullptr;
    size_t length = strlen(text);
    char *copy = malloc(length + 1);
    if (copy != nullptr)
        memcpy(copy, text, length + 1);
    return copy;
}

static void releaseEnvState(void) {
    for (size_t i = 0; i < sEntryCount; ++i)
        free(sEntries[i]);
    free(sEntries);
    free(sDebugPath);
    free(sExportPath);
    sEntries = nullptr;
    sEntryCount = 0;
    sDebugPath = nullptr;
    sExportPath = nullptr;
}

bool System_getEnvironment(const char *debugpath, const char *exportpath) {
    if (debugpath == nullptr || debugpath[0] == '\0' ||
        exportpath == nullptr || exportpath[0] == '\0')
        return false;
    size_t count = 0;
    while (environ != nullptr && environ[count] != nullptr)
        ++count;
    char **entries = count == 0 ? nullptr : calloc(count, sizeof(char*));
    if (count != 0 && entries == nullptr)
        return false;
    for (size_t i = 0; i < count; ++i) {
        entries[i] = duplicateText(environ[i]);
        if (entries[i] == nullptr) {
            for (size_t j = 0; j < i; ++j)
                free(entries[j]);
            free(entries);
            return false;
        }
    }
    char *debug = duplicateText(debugpath);
    char *exported = duplicateText(exportpath);
    if (debug == nullptr || exported == nullptr) {
        for (size_t i = 0; i < count; ++i)
            free(entries[i]);
        free(entries);
        free(debug);
        free(exported);
        return false;
    }
    releaseEnvState();
    sEntries = entries;
    sEntryCount = count;
    sDebugPath = debug;
    sExportPath = exported;
    return true;
}

const char *System_getEnvValue(const char *name) {
    if (name == nullptr || name[0] == '\0')
        return nullptr;
    size_t length = strlen(name);
    for (size_t i = 0; i < sEntryCount; ++i) {
        const char *entry = sEntries[i];
        if (strncmp(entry, name, length) == 0 && entry[length] == '=')
            return entry + length + 1;
    }
    return nullptr;
}

bool System_hasEnv(const char *name) {
    return System_getEnvValue(name) != nullptr;
}

void System_freeEnvironment(void) {
    releaseEnvState();
}
