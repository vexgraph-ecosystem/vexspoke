#ifndef EXCEPTION_THROW_H
#define EXCEPTION_THROW_H

#include <stdio.h>

// exception/throw.h — the loud-failure macro (the Failure Observability Law).
//
// THROW("...") is how a COLD rejection reports itself: one red "[vex]" line to
// stderr, prefixed with its source location, and nothing more. It never unwinds,
// never allocates, never blocks, and never returns from a caller — the
// relational model has no exception stack. A rejected operation returns its
// safe default AND THROWs the reason on the way out (the Cold-Strict,
// Hot-Minimal Validation Law).
//
// It is a MACRO by design: the source location is captured at the call site,
// so `grep -rn THROW` lists every loud rejection in the tree, exactly as
// `;;GETTER` or `;;HOTCODE` list their sites.
//
//   if (index >= count) {
//       THROW("collection readSlot: index %zu out of range (count %u)", index, count);
//       return 0;
//   }
//
// Hot paths (`;;HOTCODE`) must NEVER THROW: a log line per frame is how frames
// die (the Hot-Path Minimal Guard Law). THROW lives on cold seams only.

// __FILE_NAME__ (basename) is a compiler builtin on clang and GCC 12+; fall
// back to the full path where it is absent.
#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define THROW(fmt, ...) \
    fprintf(stderr, "[vex] %s:%d: " fmt "\n", __FILE_NAME__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#endif
