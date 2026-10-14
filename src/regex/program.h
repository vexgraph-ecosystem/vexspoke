#ifndef REGEX_PROGRAM_H
#define REGEX_PROGRAM_H

#include <stdbool.h>
#include <stdint.h>

#include "regex/nodelist.h"

// regex/program.h — the RegexProgram: the compiled instruction array, the
// Thompson construction that emits it from the AST, and the instruction
// vocabulary a matcher executes. Single Class Per File Law: RegexProgram.
//
// The program is flat and index-addressed: a SPLIT or JUMP names a target by
// instruction index, never a pointer, so the array can grow and move beneath
// the emitter without invalidating a branch it already wrote.

// Documented ceiling on one compiled program (bounds {m,n} expansion).
#define REGEX_MAX_PROGRAM 32768u

// The instruction set. Every instruction is one machine step in the simulation.
typedef enum RegexOp {
    REGEX_OP_CHAR = 0,  // consume byte
    REGEX_OP_ANY,       // consume any byte (newline unless DOTALL at runtime)
    REGEX_OP_CLASS,     // consume a byte in class bitmap a
    REGEX_OP_SPLIT,     // fork to a (priority) and b
    REGEX_OP_JUMP,      // continue at a
    REGEX_OP_SAVE,      // record the subject offset into capture slot a
    REGEX_OP_BOL,       // zero width: start of subject or line
    REGEX_OP_EOL,       // zero width: end of subject or line
    REGEX_OP_MATCH      // the program accepted
} RegexOp;

// One compiled instruction; a/b mean target index, class index or save slot
// depending on op.
typedef struct RegexInst {
    uint8_t  op;        // RegexOp
    uint8_t  byte;      // literal byte for REGEX_OP_CHAR (already folded)
    uint16_t reserved;  // keeps the record 12 Bytes and naturally aligned
    uint32_t a;         // first operand
    uint32_t b;         // second operand
} RegexInst;

// A flat, growable instruction array in the engine arena.
typedef struct RegexProgram {
    RegexInst *insts;    // engine-arena block of `capacity` instructions
    uint32_t   count;    // instructions in use
    uint32_t   capacity; // instructions allocated
} RegexProgram;

// Zero the program; allocates nothing.
void RegexProgram_init(RegexProgram *program);

// Emit the Thompson program for an AST rooted at `root`. Returns false when
// the instruction ceiling is reached or the arena refuses; the program is then
// unusable and must be freed.
bool RegexProgram_build(RegexProgram *program, const RegexNodeList *nodes, uint32_t root);

// Instructions in use.
uint32_t RegexProgram_count(const RegexProgram *program);

// Borrowed instruction, or nullptr when index is out of range.
const RegexInst *RegexProgram_at(const RegexProgram *program, uint32_t index);

// Release the instruction block and zero the program.
void RegexProgram_free(RegexProgram *program);

#endif
