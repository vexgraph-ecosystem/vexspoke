#include "system/system.h"

#include "cli/console.h"
#include "input/key.h"
#include "input/mouse.h"
#include "input/touch.h"
#include "time/nanotime.h"
#include "io/hot_file.h"
#include "annotation/definition.h"
#include "annotation/overview.h"

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
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: System (system/system.c)
 * LEVEL: L2 — Behavior (system query behavior API)
 * ============================================================================
 * Unified native system information, discovery, and display subsystem
 *
 * STRUCT FIELDS: none — procedural (operates on engine subsystems (init-once boot))
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - System_initializeAll(void)
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
