# Retained C backend during staged R2 storage migration

R2 has two cooperating owners: Vexspoke CPU computation/behavior and
Relational Engine memory/storage, stable row chunks, variable bindings and
native C search. Ownership is not proof that callers have migrated.

The current working backend remains Vexspoke's C implementation under
`src/nio`, `src/io`, `src/relational` and `src/reflection`. These source copies
were restored unchanged from the user-moved C reference in relational-engine.
Existing consumers continue using `nio/mem.h`, `io/file.h` and the existing
`Memory_*`/`File_*` interfaces, without a dependency on the scratchpad repo.

The parallel copies in relational-engine are comparison material, not a second
backend linked into ecosystem programs. The Rust package now implements a
partial storage C ABI and native C search over borrowed rows; there is no allocator
replacement or claim that today's default C functions delegate to Rust.
`src/nio/relational_memory.h` is an explicit opt-in include for the engine-owned
`relational_engine/memory.h` contract, not a default backend selection.

R1 owns code/storage residency and excludes active users before destruction.
No C/Rust atomic-layout compatibility or automatic schema migration is assumed.
GPU shaders/dispatch remain Graphvex R3. See the engine README and owner evidence
for implemented scope; imported comparison files are not standalone closure.

When the Rust implementation is ready, allocation/free and other cold operations
can be forwarded through a tested native ABI. Hot processing can continue using
borrowed native pointers/spans; it need not call Rust for each element. Preserve
header layout, matching allocation/release ownership, failure disposition,
borrow lifetime and bounded teardown before switching the backend.

Run the integrated build from the workspace root with `./tools/b build`.
Build success proves compilation/linking only. Owner tests remain necessary
for allocator, I/O, reflection, relation and concurrency behavior. b is build
truth; IDE metadata is not a runtime dependency.
