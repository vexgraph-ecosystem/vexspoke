# vexspoke, by Vex, truly.

## CLion: CMake is IDE metadata only

Open this repository root as a CMake project. `CMakeLists.txt` provides C23
source targets, include paths and compiler flags for navigation, diagnostics
and inlay hints. Its object target is excluded from the default build; no
dependency downloads, linking or application runner are wired into it.
Optional `VULKAN_INCLUDE_DIR` supplies local SDK headers. Missing headers stay
real IDE errors; no fake declarations are generated. IDE appearance is user-verified.

Build with [b](https://github.com/vex-graph/b), not this adapter. From the
Vexgraph workspace root: `./tools/b build vexspoke`. A standalone checkout
must supply its real b build/dependency context; IDE configuration is not proof
of standalone runtime readiness.

A zero-allocation, relational C23 platform library — everything is a pointer.

A play on the word **bespoke** — a *bespoken* C platform library tailor-crafted down to the cache line, register, and bit. `vexspoke` serves as the central spoke of the `vexgraph` vertical integration stack.

`vexspoke` is an absolute rejection of the traditional engine paradigm. There are no object graphs, no garbage collectors, and no hidden heap allocations. Memory is a relational table: every block knows its own type and length via a negative-offset header, every pool is a column store of equal-stride slots, and every registered symbol is a row whose value is the address of another typed block. Pointers are first-class, self-describing values — joinable without registry lookups.

The result is a lock-free, cache-coherent core with predictable, microsecond-level latency: C stripped of its comfort abstractions, rebuilt for raw, bare-metal performance.

---

## Workspace Integration & How to Use It

`vexspoke` is the **R2 behavior** layer in the supervisor order (the Vertical Integration Law) — the pure leaf shapes the R1 Kernel borrows (`MemoryArena`, events, math). Supervised by `hotcwap` at runtime, dependency of everything at compile-time. It is designed to be consumed as the foundational bedrock library within a vertically integrated ecosystem (such as `vexgraph`) or embedded standalone into custom applications.

`vexspoke` depends on nothing else in the stack; it never includes a consumer's headers. The full ecosystem map lives in the workspace root `README.md` and the ecosystem wiki, not here.

### Build

```sh
./tools/b build vexspoke # from the Vexgraph workspace root
```

### Standalone autonomy
The Standalone Autonomy Law still requires real dependency closure for runtime
builds. This IDE-only CMake adapter does not export a runtime `vexspoke` target
and never fetches dependencies; configure consumers' local header paths instead.

---

## What's in this repo

* **`src/annotation/`** — Zero-cost C23 static assert markers (`;;OVERVIEW`, `;;DRAFT`, `;;INTENTION`, `;;PLATFORM_EXCLUSIVE`).
* **`src/c23/constructor.h`** — Java-style arity constructor overloading (`Class(...)` $\rightarrow$ `Class_0`, `Class_1`) via pure preprocessor dispatch.
* **`src/nio/mem.h/.c`** — The self-describing memory lens (`Memory`). Every allocation carries `[type_id][length][payload]`. Walking back 16 bytes yields the header; `Memory_type()` and `Memory_length()` are free pointer subtractions.
* **`src/bit/bit.h/.c`** — The lockless width pool (`BitPool`). ABA-tagged freelists recycle slots; freed slots return at the *exact same address*.
* **`src/oop/type.h`** — Bit-packed type system (`Type`). One 32-bit masked ID encodes form, class, and variant.
* **`src/oop/class.h/.c`** — Reflection and stride tables for off-heap structs.
* **`src/atomic/ring.h/.c`** — Lockless MPMC ring buffer (`RingBuffer`), the inter-thread messaging highway.
* **`src/atomic/spin.h/.c`** — C23 `stdatomic` ticket locks (`SpinLock`) with bounded spin backoff.
* **`src/relational/variable.h/.c`** — Relational symbol registry (`Variable`). Name $\rightarrow$ `(classId, targetPointer)`.
* **`src/lang/`** — Zero-allocation math primitives: `FastMath`, `Vec2`, `Vec3`, `Vec4`, and `Mat4`.
* **`src/struct/`** — High-performance off-heap collections: `List`, `Map`, `Queue`, `Deque`, `Stack`, `Set`, `MinHeap`, and `SparseSet`.
* **`src/io/vexhome.h/.c`** — Canonical `VexHome` user storage layouts (`~/vex`).
* **`src/io/file.h/.c`** & **`src/io/log.h/.c`** — Zero-allocation file operations and binary event logging.
* **`src/net/`** — Zero-allocation HTTP client, URL parser, JSON serializer, and TLS streaming abstractions.
* **`src/engine/loop.h/.c`** — Fixed-timestep engine loop (`Loop`).
* **`src/vulkan/`** — Core Vulkan instance, MoltenVK loader, swapchain management, and baseline SPIR-V shaders (`hello_triangle`, `solid_quad`).
* **`src/objc/`** — Hardware platform bridges: TouchID biometric authentication, Apple SecureTransport TLS, and CoreAudio.
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
* The workspace build system, `b` (bundled at `b/`).

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
