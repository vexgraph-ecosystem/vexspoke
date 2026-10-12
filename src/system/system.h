#ifndef SYSTEM_SYSTEM_H
#define SYSTEM_SYSTEM_H

#include <stdbool.h>

// system/system.h — Unified native system information, discovery, and display subsystem
// (Legacy: darling/system/* in anti Java engine)

#include "system/discovery.h"
#include "system/display_info.h"
#include "system/display_monitor.h"
#include "system/hardware_info.h"
#include "system/graphics_info.h"
#include "system/app_detect.h"
#include "system/capture_tool.h"
#include "system/process_probe.h"

// Master initializer: boots all core engine subsystems (time, input, cli, io).
// Called exactly once at process start.
void System_initializeAll(void);

// --- Environment (the ONE process-environment handler; INTENTIONAL(vex)) ------
// The environment is explicit session context, never an ambient global read.
// System_getEnvironment binds the debug source and the manifest-derived export
// destination up front and captures the values a run sees once; every later read
// is System_getEnvValue("VAR") against that snapshot — no CWD, no hidden default.
// (See the INTENTIONAL(vex) note in system.c for why System owns this.)
bool System_getEnvironment(const char *debugpath, const char *exportpath);
const char *System_getEnvValue(const char *name);
bool System_hasEnv(const char *name);
void System_freeEnvironment(void);

#endif
