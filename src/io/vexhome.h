#ifndef IO_VEXHOME_H
#define IO_VEXHOME_H

#include <stdbool.h>

// io/vexhome.h — the VexHome class (was AntiHome, renamed on the
// vexspoke/darling split; Legacy: io/AntiHome.java).
//
// Per-user VexHome layout, created once so the engine always has a stable
// place to write before any subsystem touches the disk.
//
//   ~/vex/
//     projects/    - user project workspaces
//     logs/        - engine binary logs (Log default sink)
//     fonts/       - baked font store (FontBake .vexfont files)
//     placeholder/ - reserved scratch area
//     cache/       - per-subsystem caches (created on demand)
//       <subsystem>/
//         dictionary.ini  - living manifest index (created by VexHome_cacheEnsure)
//
// Each accessor returns a pointer to an internal static buffer (valid until
// the next call to any VexHome function).
const char *VexHome_root(void);
const char *VexHome_projects(void);
const char *VexHome_logs(void);
const char *VexHome_fonts(void);
const char *VexHome_placeholder(void);

// Per-subsystem cache accessors (static buffers, valid until next VexHome call).
// NULL or empty subsystem falls back to the root cache dir (~/vex/cache/).
const char *VexHome_cache(const char *subsystem);
const char *VexHome_cacheIndex(const char *subsystem);

// Create the cache directory and dictionary.ini manifest when absent.
// Idempotent: second call touches nothing. Never clobbers existing dictionary.ini.
bool VexHome_cacheEnsure(const char *subsystem);

// Create the full layout; idempotent. Returns true when every dir exists.
bool VexHome_ensure(void);

// Default log file: ~/vex/logs/engine.bin (truncated every run by Log).
const char *VexHome_defaultLogPath(void);

#endif