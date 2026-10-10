# vexspoke, by Vex, truly.

## Current State

**Role:** R2 CPU computation and behavior — math, algorithms, synchronization,
containers, reflection, reactive behavior, net, and the engine loop. Its
cooperating R2 storage owner is Relational Engine; R1 `hotcwap` supervises
lifetimes. An unfinished ecosystem library, not a finished product.

**Implemented and proven (macOS arm64):** the CPU/behavior surface with owner
tests under `tests/vexspoke/` — math (`FastMath`/`Vec*`/`Mat*`), algorithms
(sort/BVH/path), lockless `BitPool`/`RingBuffer`/`SpinLock`, collections
(`List`/`Map`/`Set`/`Heap`/…), the reflection hierarchy (`Variable`/`Field`/
`Struct`/`Class`/`Method`, with `Field` carrying physical layout), reactive
bindings, `net` (URL/JSON/HTTP/TLS), and the fixed-timestep `Loop`.

**Ownership moved out of this repository:** the default allocator and all
`io/*`/`nio/*` implementations live in Relational Engine; there are no `src/io`
or `src/nio` copies here. The shared **type algebra** (`MASK_*`/`PROJ_*`/
`ARCH_*`, `Type_make`, the parent resolver) now lives in Relational Engine
`type/type.h`: `src/oop/type.h` is the vexspoke registry plus that include, and
`src/oop/type.c` was removed.

**Stubbed, draft, or planned:** staged migration of the retained containers and
`src/relational` bindings to R2 storage; live engine reload integration (R1).

**Platforms proven:** macOS arm64 only; Windows is unproven.

**Evidence:** `tests/vexspoke/` owners and `tests/test-checklist.md`; the
`type_test` owner pins the registry + algebra mapping.

A C23 CPU computation and behavior library — everything is a pointer.

A play on the word **bespoke** — a *bespoken* C platform library tailor-crafted down to the cache line, register, and bit. `vexspoke` serves as the central spoke of the `vexgraph` vertical integration stack.

`vexspoke` is an absolute rejection of the traditional engine paradigm. There are no object graphs, no garbage collectors, and no hidden heap allocations. memory is a relational table: every block knows its own type and length via a negative-offset header, every pool is a column store of equal-stride slots, and every registered symbol is a row whose value is the address of another typed block. Pointers are first-class, self-describing values — joinable without registry lookups.

The result is a lock-free, cache-coherent core with predictable, microsecond-level latency: C stripped of its comfort abstractions, rebuilt for raw, bare-metal performance.

---

## Workspace Integration & How to Use It

`vexspoke` is the **R2 CPU computation and behavior** owner: math, algorithms,
synchronization and behavior APIs. Its cooperating R2 storage owner is
[Relational Engine](https://github.com/vexgraph-ecosystem/relational-engine):
memory allocation/storage, stable row chunks, variable bindings and native C
search over Rust-owned spans. R1 `hotcwap` supervises their lifetimes.

**IO/NIO ownership has migrated.** The default allocator implementation and all
`io/*` / `nio/*` headers now live in Relational Engine. Vexspoke contains no copies;
the default workspace build consumes RE. The native Memory ABI is preserved,
not rewritten into Rust. Broader collection migration remains staged.
The separate engine `nio/relational_memory.h` exposes the Rust byte/string ABI.
No C/Rust atomic-layout compatibility,
automatic schema migration or live engine reload integration is implied.

Vexspoke includes no consumer or host headers; its production R2 storage boundary
does not introduce an R1/R3/R4/R5 dependency. GPU shaders and dispatch remain
Graphvex R3. The ecosystem map lives in the workspace `../../../README.md` and
the readiness Gist. The ecosystem, especially its R5 apps, is unfinished.

### Build

```sh
./tools/b build vexspoke # from the Vexgraph workspace root
```

### Standalone autonomy
The Standalone Autonomy Law requires real dependency closure through
[b](https://github.com/vex-graph/b), including the engine-owned IO/NIO headers
and implementation. Workspace indexing does not prove standalone runtime readiness.

---

## What's in this repo

* **`src/annotation`** — Zero-cost C23 static assert markers (`;;OVERVIEW`, `;;DEFINITION`, `;;GETTER`, `;;SETTER`, `;;DRAFT`, `;;INCOMPLETE`, `;;PLATFORM_EXCLUSIVE`, `;;INTENTION`, `;;INHERITS`, `;;REACTIVE`, `;;WHAT`, `;;CHECKER`, `;;HOTCODE`, `;;DEBUG`, `;;TEST`). `;;DEBUG` marks a debug/diagnostic surface a release build may drop; `;;TEST` marks a test/inspection affordance production does not need (both C-only in their special cases; see the Two-Semicolon Annotation Style Law).
* **`src/c23/constructor.h`** — Java-style arity constructor overloading (`Class(...)` $\rightarrow$ `Class_0`, `Class_1`) via pure preprocessor dispatch.
* **Engine `src/nio/mem.h/.c`** — Production `Memory_*` ABI and self-describing header, implemented and linked from Relational Engine. No Vexspoke IO/NIO source remains.
* **Engine `src/nio/relational_memory.h`** — Separate Rust byte/string ABI include, not a silent replacement for native arena semantics.
* **`src/bit/bit.h/.c`** — The lockless width pool (`BitPool`). ABA-tagged freelists recycle slots; freed slots return at the *exact same address*.
* **`src/oop/type.h`** — the vexspoke class registry plus the shared 64-bit type-id algebra (Relational Engine `type/type.h`); one 64-bit id encodes project, form, modifier, wrappers, sugar and class.
* **`src/reflection/`** — the reflection hierarchy (`Variable`/`Field`/`Struct`/`Class`/`Method`); `src/oop/stride.h` holds the class → byte-width table.
* **`src/atomic/ring.h/.c`** — Lockless MPMC ring buffer (`RingBuffer`), the inter-thread messaging highway.
* **`src/atomic/spin.h/.c`** — C23 `stdatomic` ticket locks (`SpinLock`) with bounded spin backoff.
* **`src/relational/`** — Retained C relational bindings and symbol operations; migration to the R2 storage owner requires separate proof.
* **`src/lang`** — Zero-allocation math primitives: `FastMath`, `Vec2`, `Vec3`, `Vec4`, and `Mat4`.
* **`src/struct`** — High-performance off-heap collections: `List`, `Map`, `Queue`, `Deque`, `Stack`, `Set`, `MinHeap`, and `SparseSet`.
* **IO/NIO** — implemented in Relational Engine; this repository keeps no `src/io` or `src/nio` sources.
* **`src/net`** — Zero-allocation HTTP client, URL parser, JSON serializer, and TLS streaming abstractions.
* **`src/engine/loop.h/.c`** — Fixed-timestep engine loop (`Loop`).
* **Graphvex R3** — GPU resources, Vulkan pipelines, shaders and dispatch belong to the graphics owner, not this CPU library.
* **`src/objc`** — Hardware platform bridges: TouchID biometric authentication, Apple SecureTransport TLS, and CoreAudio.
* **`src/main/main.c`** — Standalone headless harness: 4 concurrent producer threads racing into a shared ring, verified at `received=100/100 ticks=1`.

---

## Architecture in brief

* **Self-describing memory** — Every pointer carries its own `type_id` + `length` negative-offset header. A raw pointer *is* a typed, introspectable value.
* **Zero steady-state allocation** — The arena doctrine carves memory once from the OS; pools, rings, and tables recycle memory in-place. Zero `malloc` in the frame loop.
* **Lockless concurrency** — Inter-thread work is distributed through ABA-tagged atomic slots and compare-and-swap (CAS), never blocking mutexes.
* **Relational joining** — Symbols resolve to typed addresses directly; relational queries join memory blocks without object graphs.
* **Strict C23 dialect** — banned `->` arrow sugar (the Semantic Consistency Law (Reference form)), the two-layer access cap (the Semantic Consistency Law (Access depth)), and destination-last parameter order (the Semantic Consistency Law (Argument order)).

---

## Embracing the Pointer & Banning Pointer Chasing (the Semantic Consistency Law (Access depth))

In high-level languages like Java, developers write:
```java
Car car = new Car();
```
Beginners often treat `car` as if it were a direct, inlined struct. But in the JVM and physical hardware registers, **`car` is never a struct—it is purely an object reference, a pointer under the hood**. 

In `vexspoke`, we stop pretending and **embrace the pointer directly**. Everything is a pointer.

### The Pointer Chasing Trap
When languages allow unchecked dot-chaining (`car.engine.turbo.valve.pressure`), software falls into the trap of **pointer chasing**:
* Pointer A hops to pointer B...
* Pointer B hops to pointer C...
* Pointer C hops to pointer D across distant, unpredictable cache lines.

Every hop in that chain is an unmeasured memory dereference that risks CPU pipeline stalls, cache line thrashing, and cognitive drift where the developer forgets the physical cost of memory traversal.

### One Level + Offset: That's How Simple It Is
In physical hardware, the fastest, most predictable memory access is fundamentally:
$$\text{Effective Address} = \text{Base Pointer} + \text{Offset}$$

That is precisely what `(*ptr).field` expresses:
1. `*ptr` explicitly dereferences the base pointer once to anchor the record.
2. `.field` applies the compile-time struct byte offset to reach the value.

### The Two-Layer Access Cap (the Semantic Consistency Law (Access depth))
To eliminate pointer chasing across the entire ecosystem, `vexspoke` strictly enforces the **Two-Layer Access Cap**:
```c
(*layer1).layer2             // yes — base hop + offset (one level + offset)
(*p).items[i]                // yes — base hop + indexed offset
(*(*ptr).field).field2       // NO — three layers (pointer chasing)
obj.field.field2.field3      // NO — three layers (pointer chasing)
```

If you need a member from an inner record, you **must hoist the intermediate into a local variable first**:
```c
Engine *e = (*car).engine;
Valve *v = (*e).valve;
(*v).pressure = 120.0f;
```
By forcing the intermediate pointer into a local:
1. **Explicit Cost**: Every memory boundary crossed is physically visible to both the human architect and the AI agent.
2. **Register Locality**: Intermediate base pointers are hoisted into CPU registers, avoiding redundant indirection.
3. **Zero Mental Drift**: You never chase pointers into the dark; memory remains mechanical, observable, and cache-coherent. That's how simple it is.

---

## Requirements

* A modern C23 compiler (Clang recommended, `-std=gnu23` enabled).
* Apple Silicon (arm64 macOS 14+) or Linux.
* Vulkan SDK (MoltenVK on macOS).
* The workspace build system, `b` (bundled at `../../../personal/b`).

---

## Building & Verification

To build and run the standalone verification harness:

```sh
./tools/b run vexspoke        # inside the worktree: build + run the harness
# or, standalone:  b build c .   then run the produced binary
```

**Expected output:**
```
== vex memory ==
type=0x20000001 len=16
== vex bit pool ==
recycled a => c=0x10199ec60 (same=1)
== vex ring + spin + loop ==
...
received=100/100 ticks=1
```

Enforced compilation flags: `-Wall -Wextra -Werror` with C23 (`-std=gnu23`) and `-mcpu=apple-m1` (the Platform Support Floor Law); `-mcpu=native` is local-dev only, never shipped.

---

## Scope and Limitations

**Scope:** R2 CPU computation and behavior: math, algorithms, synchronization,
collections, reflection, reactive behavior, the networking client and the engine
loop. It provides the CPU contract the higher tiers build on.

**Deliberately not covered:**
- No memory/IO implementation: the allocator and `io/*`/`nio/*` live in
  Relational Engine; no `src/io`/`src/nio` copies remain here.
- No GPU work (Graphvex R3), no host/window (hotcwap R1), no UI (darling R4).
- No type-algebra ownership: masks/`PROJ_*`/`ARCH_*` and the parent resolver live
  in Relational Engine `type/type.h`.

**Known limits and gaps:**
- Collection/relational migration to R2 storage is staged; retained C surfaces
  remain until migrated with owner proof.
- Proven only on macOS arm64; Windows is untested.
- No live engine reload integration.
