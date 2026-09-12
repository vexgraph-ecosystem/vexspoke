#ifndef RELATIONAL_RELATIONAL_H
#define RELATIONAL_RELATIONAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "relational/variable.h"

// relational/relational.h — spotlight relational facade over Variable (Legacy: relational/RelationalEngine.java).
//
// RELATIONAL ENGINE PHILOSOPHY: this engine refuses the textbook trilemma —
// book-OOP vs DOD vs ECS — because none of them answers the question it
// exists for: "find the thing called X, right now, from anywhere."
//
//   Book-OOP binds names at COMPILE time (identifiers vanish into addresses)
//     and hides state behind encapsulation. At runtime nothing is findable
//     except by walking graphs you must already hold. Query cost: O(graph).
//   DOD answers "process everything fast" (sweeps over flat arrays). It
//     never answers "find one thing now" — you rebuild that per case. The
//     engine does not compete: hot iteration stays DOD (scene graphs, SoA
//     physics); cold rendezvous comes here. Complementary axes.
//   ECS answers "all entities with [A,B,C]" — sets by signature, entities
//     as numbers. It never answers "the thing called character.position.x"
//     without a bolted-on name table, i.e. this engine reinvented badly.
//     ECS shards values for systems slicing; the engine maps names to whole
//     values for authors addressing things. Different questions.
//
// The thesis: OOP names things for the compiler, ECS numbers things for the
// scheduler — the relational engine names things for everyone at runtime
// (authors, debuggers, scripts, search boxes, hot-swap, telemetry), with
// O(log n) name resolution plus O(1) slot->row->value hops and O(n) class
// filters. One primitive (name => value, globally findable) underlies N
// features — spotlight, live inspectors, script binding, save/load walks,
// swap rebinding — instead of N bespoke lookup systems.
//
// Honest costs, paid deliberately:
//   - Runtime names mean runtime typos (the compiler stops checking so the
//     runtime can start finding). Paid with: strict charset, fail-closed
//     lookups, class tags pinned at creation, loud constructor errors.
//   - A global writable namespace invites spooky action. Paid with scopes:
//     search spans global+local, mutation stays scope-local, and the strict
//     constructor refuses silent resurrection.
//   - No overclaim: gather-by-name is O(log n), not O(1) end to end; class
//     filters are O(n) integer scans for cold sweeps only. Hot per-frame
//     typed iteration belongs to scene graphs, never here.
//
// Two Variable tables are the scopes: global and local. Every symbol is a row
// name => (classId, targetPointer). The pointer is the value — a string block,
// a typed struct, a Map, or a function address. Search is the spotlight: query
// "health" returns health, health_ui, health_progress_bar, hp_text, etc.

// Exact lookup: name => varId, or -1 if absent.
int32_t Relational_getId(Variable *scope, const char *name);

// Name accessors for search/rename. varId must be valid.
int Relational_getName(Variable *scope, int32_t varId, char *out, size_t outCap);
bool Relational_setName(Variable *scope, const char *oldName, const char *newName);

// Value accessors — store as-is, no copy. classId describes the pointer type.
void *Relational_getValue(Variable *scope, const char *name);
void *Relational_getValueById(Variable *scope, int32_t varId);
bool Relational_setValue(Variable *scope, const char *name, uint32_t classId, void *ptr);
bool Relational_setValueById(Variable *scope, int32_t varId, void *ptr);

// String sugar — value is a string block (TYPE_STRING_ARRAY). Old block is freed on set.
const char *Relational_getString(Variable *scope, const char *name);
void Relational_setString(Variable *scope, const char *name, const char *value);

// Function pointers — same as value, typed as void (*)(void*).
void *Relational_getFunction(Variable *scope, const char *name);
bool Relational_setFunction(Variable *scope, const char *name, void *fn);

// Spotlight search: query substring (case-insensitive) over all names in scope.
// Fills outIds[cap] with matching varIds ranked exact > prefix > substring.
// Returns the number of matches (may exceed cap; only cap are written).
size_t Relational_search(Variable *scope, const char *query, int32_t *outIds, size_t cap);
size_t Relational_searchAll(Variable *global, Variable *local, const char *query, int32_t *outIds, size_t cap);

#endif
