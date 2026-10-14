#ifndef REGEX_VM_H
#define REGEX_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "regex/charclass.h"
#include "regex/program.h"
#include "regex/span.h"

// regex/vm.h — the RegexVm: the Pike-VM simulator that executes a compiled
// program over a subject. Single Class Per File Law: RegexVm.
//
// The simulation is the reason this engine cannot hang: work is bounded by
// (subject length x program length) with NO backtracking and NO recursion, and
// everything it needs is allocated once, before the run. A thread list carries
// the still-live NFA states; each capture set lives in a row of the capture
// table; and the epsilon-closure walks a work stack instead of the C stack.
// The winning capture row is copied out the moment MATCH is reached.

// The match-time options, resolved from the public REGEX_* flag word by
// regex/regex.c so this unit never depends on the public header.
typedef struct RegexOptions {
    bool ignoreCase;  // fold the subject byte when comparing REGEX_OP_CHAR
    bool dotall;      // REGEX_OP_ANY also consumes newline
    bool multiline;   // '^'/'\$' also match at line edges
} RegexOptions;

// One closure work item: either "expand this pc" or "restore a capture slot
// after the subtree beneath it finishes" (so sibling branches stay clean).
typedef struct RegexTask {
    uint32_t pc;       // program counter to expand (restore items: 0)
    uint32_t slot;     // capture slot to restore
    size_t   value;    // previous value of that slot
    uint8_t  restore;  // 1 = this is a restore marker
} RegexTask;

// Per-run scratch, sized once from the program. One row per thread slot in
// each of two capture regions, a shared all-unset seed row, and the copied
// winner. Immutable except for `best` after RegexVm_init.
typedef struct RegexVm {
    const RegexProgram *program;       // borrowed instruction source
    uint32_t   *clistPc;               // current thread list: program counters
    uint32_t   *clistCap;              // current thread list: capture rows
    uint32_t   *clistSeen;             // current list's per-position marks
    uint32_t   *nlistPc;               // next thread list: program counters
    uint32_t   *nlistCap;              // next thread list: capture rows
    uint32_t   *nlistSeen;             // next list's per-position marks
    RegexTask  *stack;                 // epsilon-closure work stack
    size_t     *caps;                  // two capture regions (2 x capRowCount)
    size_t     *work;                  // the closure's working capture row
    size_t     *seed;                  // the all-unset row every search starts from
    size_t     *best;                  // the winning capture row
    uint32_t    capStride;             // size_t slots per capture row
    uint32_t    capRowCount;           // rows per region
    uint32_t    stackCapacity;         // work-stack entries
} RegexVm;

// Allocate every scratch block up front; a run allocates nothing more. False
// when the arena refuses, in which case the VM is unusable.
bool RegexVm_init(RegexVm *vm, const RegexProgram *program, uint32_t groupCount);

// Release every scratch block and zero the VM. Null-safe.
void RegexVm_free(RegexVm *vm);

// Run the simulation. anchored restricts the match to offset 0. Returns true
// on a match and copies the winning capture row into the VM's best row.
bool RegexVm_run(RegexVm *vm, const RegexCharClass *classes, const char *subject,
                 size_t length, bool anchored, RegexOptions options);

// The winning capture span for one group (0 = the whole match); an unmatched
// group yields RegexSpan_none(). Valid after a successful run.
RegexSpan RegexVm_span(const RegexVm *vm, uint32_t group);

#endif
