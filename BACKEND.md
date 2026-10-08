# Production R2 storage migration

Relational Engine now owns production native `src/nio` and `src/io`, including
the macOS clipboard adapter. Vexspoke contains no IO/NIO copies. The default
workspace build links `librelational_engine.a`; canonical `nio/mem.h` and
`io/file.h` resolve from engine sources. CPU computation/behavior remains Vexspoke.

The existing native `Memory_*`/`MemoryArena_*`/`Transient_*` and `File_*` ABI,
16-byte allocation headers and arena semantics are preserved. This is not a Rust
allocator rewrite: engine Rust typed pools and its partial storage C ABI are
separate surfaces. Broader container migration remains staged. Engine
`nio/relational_memory.h` exposes `relational_engine/memory.h` separately.

Vexspoke retains `src/relational` and `src/reflection`; engine copies of those
directories are imported reference-only. CPU include paths precede engine paths
for consumers, so those reference headers never shadow Vexspoke's contracts.
Engine native IO/NIO borrows CPU spin/crypto/annotation/type contracts without a
recursive build dependency or duplicate storage implementation.

R1 owns code/storage residency and excludes active users before destruction.
No C/Rust atomic-layout compatibility or automatic schema migration is assumed.
GPU shaders/dispatch remain Graphvex R3; live Hot loader integration is unproved.

Run `./tools/b build` from the workspace root. Production provenance is tested by
`python3 tests/vexspoke/backend_contract_test.py`; native behavior and ASan/UBSan
by `python3 tests/relational-engine/native_run.py`. Clipboard mutation skips unless
explicitly permitted. HotFileSys remains a draft no-op. Build success alone is
not complete per-file, concurrency, other-platform or visual proof.
