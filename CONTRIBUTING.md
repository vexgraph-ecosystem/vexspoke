# Contributions & Engineering Manifesto (vexspoke)

This project is a strictly solo development process conducted in tight pair-programming partnership with an AI coding assistant.

It serves as an architectural manifesto for **Level 4 Relational Memory Substrate**: an uncompromising low-level engine where **everything is a pointer**, every memory block carries a self-describing bit-packed header, and steady-state allocation is zero.

---

## 1. The AI-First Architecture Manifesto & Boilerplate Defense

This codebase strictly enforces the verbose, explicit boilerplate required across the `vexgraph` ecosystem:
- Strict prohibition of arrow syntax (`p->field` is banned; only explicit `(*p).field` is permitted).
- Single Class Per File (the Java Law: one public `typedef struct` per `.h`/`.c` pair).
- Arity-overloaded explicit constructor dispatch macros (`Class_0()`, `Class_1()`).
- Complete, symmetric getters and setters for all struct fields.
- Strict dest-last parameter ordering `(a, b, dest)`.
- Two-layer member access cap (`(*layer1).layer2` maximum).
- Exhaustive `;;OVERVIEW` blueprints mirrored at the top of every implementation file.

### Why the Boilerplate Exists
This boilerplate is **not** an accident, nor is it a misunderstanding of idiomatic C. It is an intentional, machine-verifiable scaffold built specifically for **AI-Human Pair Systems Programming**:
1. **Machine Comprehension**: Eliminating `->` and isolating classes to single files allows an AI coding agent to reason over raw pointer offsets, bit-packed headers, and ring buffers with mathematical precision and zero aliasing.
2. **Explicit Indirection**: `(*ptr).field` ensures every memory hop is laid bare in the source code.
3. **AI-Maintained Rigor**: The AI agent authors and maintains the dense boilerplate, allowing human architectural guidance to focus on lockless concurrency, cache line alignment, and ABA prevention.

---

## 2. Sanity Warning for External Contributors

> [!WARNING]
> **SANITY NOTICE FOR EXTERNAL CONTRIBUTORS**
> This repository is not designed for traditional C conveniences, casual hacking, or stylistic shortcuts. It is an unapologetic, machine-verifiable manifesto of AI-augmented systems architecture.
>
> **If you do not approve of this architecture or cannot find peace with this philosophy, consider leaving this repository for your own sanity.**
>
> We do not accept Pull Requests, issues, or unsolicited stylistic refactors attempting to re-introduce `->`, combine multiple classes into one file, or strip out memory headers. Upstream is maintained exclusively by the author and the AI agent.

---

## 3. Supreme Living Document: `preferences.md`

All architectural rules and style invariants are governed by the central constitution in the root workspace:

👉 **[vexgraph / preferences.md](../../preferences.md)**

Whenever preferences or conventions evolve, [`../../preferences.md`](../../preferences.md) is updated and committed locally in the same cycle (Zero Drift Law).

---

## 4. Architectural Principles


| Principle | Specification |
| :--- |:---|
| **Everything is a pointer** | Symbols resolve to addresses, addresses decode themselves; no wrappers, no handles-to-handles. |
| **Self-describing memory** | Every block carries a bit-packed `type_id` + `length` header; `Memory_type()`/`Memory_length()` are free. |
| **Zero steady-state allocation** | One arena carved from the OS; no `malloc` in ticks, physics, audio, or networking paths. |
| **Lockless subsystems** | ABA-tagged freelists, CAS spinlocks, MPMC rings. No mutexes in the hot path. |
| **Atomic Commit Discipline** | Per-file atomic git commits with explicit scopes (`feat`, `refactor`, `perf`, `chore`, `style`). |

---

## 5. Banned Patterns & Permitted Replacements

| Banned Pattern | Reason for Ban | Permitted Replacement |
|:---|:---|:---|
| Arrow member access (`p->field`) | Hidden indirection, against the doctrine. | `(*p).field` — always explicit. |
| Casts with no space (`(int)p`) | Style rule. | `(int) p` — exactly one space after `)`. |
| Pointer declarator `Type* name` / `Type * name` / `void*` in decl, or `(Type *)` with space inside cast | Breaks `T *name` in decls and `(T*)` in casts. | `Type *name`, `void *data` in decls (space before `*`, `*` binds to name); `(Type*) ptr` in casts (no space before `*` inside `( )`, one space after `)`). |
| `typedef struct _x { } x;` | Non-class naming. | `typedef struct Class { } Class;` — same tag and typedef. |
| Mixed-caps or snake-capital functions | Style rule. | Definition `functionName`, call site `Class_functionName(...)`. |
| Braced single-statement `if` | Style rule. | `if(foo)` newline `(*coo).doo(params);` — no braces. |
| Dest-first `Func(dest, a, b)` | Violates dest-last `(a, b, dest)`. | `Vec4_add(a, b, dest)` / `Mat4_multiply(left, right, dest)` — result last. |
| Deep member chain `(*(*ptr).f).g` / `a.b.c` (>2 layers) | Violates 2-layer cap, unreadable. | Hoist: `Field *f = &(*layout).items[i];` then `(*f).offset`. |
| External GUI/windowing libs (GLFW, SDL) | Pulls in C++/extra dependencies, contradicts the native backend. | `window.Window` — AppKit directly via Objective-C shim. |
| Annotation marker written bare (e.g. `DRAFT`) | Style rule. | `;;DRAFT` — two semicolons prefix, nothing after. |
| Rendering `contentPanel` into swapchain / double-rendering IOSurface panels | Violates layer order & No Double-Render law; `contentPanel` is placeholder only. | Only `TYPE_SCENE*` to swapchain; IOSurface children via `CALayer` (`Window_resizePanelIOSurface`). |
| `IOSurface` / Vulkan size in logical points | Blurry on Retina. | `px = (int)(points * scale + 0.5f)`, `CALayer` frame in points, `contentsScale = backingScaleFactor`. |
| `IOSurface` panel without `contentsGravity`/`anchorPoint` from `selfAnchor` | Content drifts on live resize. | Set `contentsGravity`/`anchorPoint` per `selfAnchor` map + `geometryFlipped = YES`. |

---

## 6. Memory Tier Architecture

| Pool | Slot Size | Managed Types |
| :--- | :--- | :--- |
| **`BitPool`** | parameterized | any equal-stride slot: primitives, structs, ring cells. |
| **`Memory`** | 16B header + payload | self-describing typed blocks (`Memory_alloc`). |
| **`RingBuffer`** | N cells | MPMC inter-thread channel. |
| **`Variable`** | 48B rows | relational symbol registry: name → classId + pointer. |

---

## 7. Local Build & Verification Workflow

| Step | Command | Expected Result |
| :--- | :--- | :--- |
| **1. Configure** | `cmake .. -DCMAKE_BUILD_TYPE=Debug` | configure succeeds. |
| **2. Build (strict)** | `cmake --build .` | all targets, `-Werror`, zero warnings. |
| **3. Engine Demo** | `./anti` | `received=100/100 ticks=N`. |

Commit history is per-file and granular — "this is what I did" — even if
intermediate commits don't compile. Pushing happens only on my explicit say-so.

