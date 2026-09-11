# vexgraph's vexspoke — C23 Engine & Multi-Repo Preferences

The engine is a relational system where **everything is a pointer**.
These rules are hard requirements, not suggestions.

## Separation of Concerns: Rule Taxonomy

To ensure uncompromising architectural consistency across all repositories and contributors (human and AI), the rules are partitioned into three distinct tiers of concern, each defined with its architectural **Definition** and foundational **Why**:

1. **Tier 1: Critical Architectural Invariants & Memory Consistency (Non-Negotiable Core)**
   - *Concern*: Hardware execution safety, zero steady-state allocation, lifetime predictability, thread safety, and crash prevention.
   - *Rules*: Rule 3 (Single Class Per File / Java Law), Rule 6 & 20 (Atomic Commits & Upstream-First), Rule 11 & 16 (Two-Layer Compositing Split & No Double-Render), Rule 13 (Apple Silicon Native), Rule 26 (Teardown Order: Destroy Top-Down, Free Last), Rule 27 (Bounded Waits on Joined Threads), Rule 28 (System Levels L1–L4, distinct from R1–R5 Supervisor Order), Rule 35 (Cold-Strict Crash-Guard half: never crash/block/allocate/use-after-free).
   - *The Why*: Violations cause segmentation faults, thread deadlocks, memory leaks, GPU driver crashes, or un-bisectable repositories.

2. **Tier 2: Semantics, Object Models & Living Contracts**
   - *Concern*: Relational memory layout, object-oriented encapsulation in pure C23, deterministic constructor dispatch, symmetric introspection, and self-documenting code contracts.
   - *Rules*: Rule 9 (Dest-Last), Rule 10 (Two-Layer Access Cap), Rule 14 (Constructor Dispatch Macro), Rule 17 (Supervisor Order R1–R5), Rule 21 (API Independence), Rule 23 (;;OVERVIEW Living Blueprint), Rule 24 (Symmetric Getter/Setter Completeness), Rule 29 (Sub-Part Field Segregation & `Class_part_verb`), Rule 30 (Living Darling Docs), Rule 31 (AI-First Architecture Manifesto), Rule 32 (Living Preferences Law), Rule 33 (Conflict Triage — Managed Exception, Not Veto), Rule 35 (Hot-Minimal contract half: setter validation policy, truncation flag, seam tests).
   - *The Why*: High-level C code must act as a reliable, predictable class system. Every struct field must have transparent, symmetric access; every class must be fully documented in-place.

3. **Tier 3: Syntactic Aesthetics & Mechanical Determinism**
   - *Concern*: Eliminating ambiguous syntax, visual sugar, and aliasing that obscures pointer operations or impairs machine readability.
   - *Rules*: Rule 1 (No Arrow Sugar: `(*ptr).field`), Rule 2 (Cast Spacing: `(T*) var`), Rule 4 (Function Naming), Rule 5 (Bracketless Single-Line If), Rule 7 (No Auto-Pushing), Rule 8 (Two-Semicolon Annotation Style), Rule 12 (No Implicit Types), Rule 25 (Per-Repo Commit Message Scope).
   - *The Why*: The codebase is engineered for AI-human pair systems programming. Machine reasoning thrives on explicit, un-sugared syntax where every dereference is visible and unambiguous.

---


## 1. No arrow sugar
Never use `->`. Field access is always `(*ptr).field`.

```c
(*this).x = 5;            // yes
this->x = 5;              // no
```

## 2. Casts: space after the closing paren
A cast has exactly one space to the right of `)`.

```c
uintptr_t addr = (uintptr_t) ptr;      // yes
uintptr_t addr = (uintptr_t)ptr;       // no
```

## 3. Class structs (One Class Per File — Java Law)
A class is `typedef struct Class {} Class;` — same name for tag and typedef.

```c
typedef struct SpinLock { ... } SpinLock;   // yes
typedef struct _spin_lock { ... } SpinLock; // no
```

One public class struct per `.h`/`.c` pair, period. File name is the
lowercase class name (`spin.h`/`spin.c`, `hot_module.h`/`hot_module.c`).
A second public `typedef struct` in the same file is a defect, no matter
how small — split it into its own file pair. One problem in one file
stays in one file and never risks the whole architecture.

Private file-local helpers (`static` structs with no behavior, or pure-data
sub-records with no `Class_*` functions) are the sole exception: they stay
in the owning `.c` and must never be included by another file. If kept, the
overview MUST list each helper with its full field list (type, name, and
one-line role) under `PRIVATE HELPERS` — same field detail as the main
class, so the file's whole memory layout reads from the header alone.
Anything with behavior
(`trampoline_register/get/set`, `hot_retire_handle`, `find_module`, ...)
is a class and gets its own file.

Slot records are the one co-location allowed: a behaviorless row/entry
struct owned by exactly one class (its table/ring/pool) may live in that
class's file pair, documented as `SLOT RECORD` in the overview. It must
have zero `Class_*` functions of its own — all behavior hangs off the
owning class (`HotTrampoline` row vs `HotTrampolineTable`,
`HotRetiredHandle` vs `HotRetireRing`). Two classes with behavior never
share a file; one class plus its dumb slots may.

The overview's `STRUCT FIELDS` section always mirrors exactly the file's
own class — the struct matching the file name — field-for-field, in
declaration order. No foreign class fields there; kept helpers live only
under `PRIVATE HELPERS` with their own fields.

## 4. Function naming
- The symbol name in the source is lowercase camelCase (`functionName`).
- It is called as `Class_functionName(params)`.

```c
static uint32_t ticket(uint64_t thread_id) { ... }   // definition

uint32_t t = SpinLock_ticket(0);                    // call site
```

## 5. Single-line if bodies
An `if` with one statement uses no braces — a bare one-liner on the next line.

```c
if(foo)
    (*coo).doo(params);
```

Multi-statement bodies always use braces.

## 6. Cohesive Commits: Per Feature, Per Subsystem/Class, Per Repository
Commits must be strictly cohesive and buildable: **one logical feature or subsystem unit per repository**. Because the ecosystem consists of multiple specialized repositories that are assessed independently, git histories must be clean, traceable, and fully bisectable.
- **Per Repository**: Commits must be executed locally inside the specific repository's git root (`projects/<repo>`). Never cross repository boundaries in a single commit, and never bundle multiple repos into one untracked commit.
- **Atomic & Bisectable**: Every commit MUST compile cleanly (`-Wall -Wextra -Werror`) and include its build wiring (`CMakeLists.txt`). Broken intermediate states or dead unwired commits are defects that ruin `git bisect`.
- **Cohesive Scope**:
  - Independent classes or isolated fixes commit individually: `feat(cursor): ...`, `fix(label): ...`.
  - Tightly coupled class pairs or cohesive subsystems landing together (e.g. `mesh` + `meshlet`, `brush` + `raster_brush`, `fence` + `semaphore` + `command_buffer`) commit together as a unified functional unit: `feat(mesh): ...`, `feat(paint): ...`, `feat(sync): ...`.
  - Never bundle multiple unrelated subsystems into a single untracked omnibus blob.
- Cross-cutting dependencies commit **upstream-first** per Rule 20 (`vexspoke` -> `graphvex`/`api-haven`/`language`/`darkbase` -> `hotcwap` -> `darling-framework`/`sesh` -> `vexgraph`).
Per-file means per-class file pair: one commit lands the owning `.h` plus its `.c` plus build wiring plus its `;;OVERVIEW` update together and must compile `-Wall -Wextra -Werror`. A literal single-file behavior commit that leaves its pair unbuildable is a defect. Exception (L1-only, Rule 33): comment/docs/manifest-only single-file commits with zero struct/API change that compile cleanly are permitted with class scope. Stack large pairs instead of splitting them.


## 7. No auto-pushing
Never run `git push` on your own. When I explicitly tell you to "push", treat it as a one-off command: execute a single `git push` to sync the repository, and then immediately revert to your default state of never auto-pushing. Regardless of pushing, you must always continue making local, granular commits for every completed feature.

## 8. Annotation style
Annotations (src/annotation/*.h) are written with two semicolons on the left
side only, so they read as explicit markers:

```c
;;DRAFT
;;INCOMPLETE
;;PLATFORM_EXCLUSIVE("Windows")
;;INTENTION("reason")
```

Two semicolons on the left, nothing on the right — even when the annotation
carries params. The semicolons are plain null declarations; the marker macro
inside expands to a `_Static_assert` that validates the annotation text.

## 9. Dest last
Output parameters come LAST: `(a, b, dest)` / `(left, right, dest)`. Reads
left-to-right like math; the result lands where it belongs, at the end.

```c
Vec4_add(a, b, dest);        // yes
Mat4_multiply(left, right, dest); // yes
Vec4_add(dest, a, b);        // no
```

## 10. Two-layer access cap
A member/index chain touches at most TWO layers deep:

```c
(*layer1).layer2             // yes — the deepest a chain goes
(*p).items[i]                // yes — indexing rides on its base hop
(*(*ptr).field).field2       // no — three layers
obj.field.field2.field3      // no — three layers
```

Anything deeper must hoist an intermediate into a local first
(`Field *f = &(*layout).items[i];` then `(*f).offset`).

### The Rationale (Java Object References & Eliminating Pointer Chasing):
In Java, an object reference (`Car car = new Car();`) is never an inline struct; it is purely a pointer under the hood. Instead of hiding behind syntactical illusions or garbage collection, `vexspoke` **embraces the pointer directly**.
When high-level languages allow arbitrary dot-chaining (`car.engine.turbo.valve.pressure`), software falls into the trap of **pointer chasing**—drifting from address to pointer to pointer across disparate memory pages, thrashing CPU cache lines and obscuring memory latency.
Physical hardware memory access is fundamentally simple: **one level + offset**. That is precisely what `(*ptr).field` is: `base_address + field_offset`.
By capping access to at most two layers, pointer hops remain explicit, measurable, and bounded. Accessing a deeper child requires hoisting it into a local variable (`Engine *e = (*car).engine;`), making every memory hop deliberate, visible in machine registers, and impossible to overlook. That's how simple it is.

## Reminders
- `-Wall -Wextra -Werror`, `-mcpu=native` (host apple-mN; portable across Apple Silicon — baseline `apple-m1`/`generic` if strict M1 compat needed), C23 (gnu23).
- Files are lowercase (`variable.c`, `spin.h`); classes are CapitalCase.
- Structs act as classes: state lives on the struct, behavior lives in
  `Class_functionName(struct Class *self, ...)`.

---

## 11. Window / Compositing Layer Order (macOS)
The compositing stack from top to bottom is fixed:

```
IOSurfaces          ← floating panels (HUD, mini-3D, etc.) via CALayer
Scene Panel         ← optional Vulkan scene stamped on swapchain (NULL = transparent)
NSVisualEffectView  ← blur / vibrancy (behind-window blending)
Window Frame        ← NSWindow chrome / traffic lights
```

- `contentPanel` is a **pure placeholder**: its own background color is ignored.
  Children of `contentPanel` get individual `IOSurface`-backed `CALayer`s composited
  by AppKit. Never render `contentPanel` itself into the swapchain.
- `scenePanel` (set via `Window_setScenePanel`) renders directly into the Vulkan
  swapchain background. `NULL` means the swapchain clears to fully transparent,
  letting blur show through.
- Calling `Window_setBlur(w, value > 0)` **must** also mark the window transparent
  so Vulkan rebuilds the swapchain with `VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR`.
  This is done automatically inside `Window_setBlur`.
- **Pane-of-glass rule (Rule 11.5):** a scene child may own a "pane" — its OWN
  `CAMetalLayer` + dedicated per-pane Vulkan swapchain (`VkPane` registry in
  hotcwap, `PanelCocoa_newMetal` in darling) — instead of an IOSurface blit.
  Each pane's layer frame is a logical-points rect pinned by `selfAnchor`
  (Rule 13 gravity applies identically). The pane renders at 60fps into its own
  swapchain (`VkPane_presentAll` from `Kernel_tick`); the board swapchain is
  never rebuilt by a pane. A pane's pixel size is FIXED at
  register/resize time — `VkPane_resize` is a no-op when the requested size is
  unchanged, so fixed panes never rebuild.
- **Live resize is layer-motion only on thread 0, and the scenes stay alive
  on the present worker (Rule 11.6):** during an active AppKit drag
  (`NSViewLiveResize`) thread 0 is inside the modal tracking loop — it moves
  CALayer frames and nothing else. Presentation runs on a dedicated present
  worker (`Kernel`'s `kernel_present_job` / `Application`'s `app_present_job`,
  spawned after window warm-up): it keeps presenting what Vulkan already has —
  the board swapchain stays at its current extent and the `CAMetalLayer`
  scales it to the live frame, and the panes keep RENDERING + presenting at
  60fps into their fixed chains (`VkPane_presentAll` never pauses), so the
  four scenes keep animating the whole drag. The board layer's
  `contentsGravity` flips to `kCAGravityResize` for the drag — the frozen
  board frame STRETCHES to cover the live bounds every drag step, so the
  interior tracks the window edges (no empty gorge past the frozen extent,
  no perceived resize lag); it flips back to `kCAGravityTopLeft` on settle
  for the exact-size rebuild. Static IOSurface children hold
  their last render. Pane/corner anchor motion during the drag is
  WindowServer-accelerated: `PanelCocoa_setAnchors` sets each layer's
  `autoresizingMask` + `anchorPoint` at attach, so CoreAnimation lays the
  sublayers out INSIDE the window-resize transaction — edge-locked on the
  same vsync as the window edge, zero CPU math, zero catch-up.
  `Window_compositeIOSurfaceChildren` early-returns while
  `Window_isLiveResizing` (a per-event explicit frame would fight the
  accelerated autoresize pass and trail the live edge by a beat — the
  right/bottom "catching up" artifact); exact frames are re-applied at
  settle. `Darling_preFrame` still
  early-returns while `Window_isLiveResizing`, so the worker never mutates
  container layout or composites concurrently — a worker/thread-0 layout race
  tears the `selfAnchor` math and pane layers drift from their pinned corners.
  Zero swapchain rebuilds and zero IOSurface re-records
  per drag frame (`Window_isLiveResizing` gates both `presentFrameLocked`,
  `renderNativeContent`, and `Darling_preFrame`; `Window_width/height` are
  atomic caches written
  by thread 0 in `setFrameSize`/`windowWillResize`/`windowDidResize`/the
  pollEvents pump and read by the worker). On settle
  (`viewDidEndLiveResize`) the flag clears, the final drawableSize lands, and
  the next pass runs exactly ONE rebuild + ONE re-record at the true final
  size. Rebuilding or re-recording per drag frame is the
  size-proportional-lag defect — large windows lag more. `Kernel_run`
  bounded-joins the worker (`Thread_stop`) before teardown per Rule 26/27.
- Pane register timing: `Darling_initCompositor` runs `Vk_init` first; child
  attach happens inside `Darling_preFrame` → `renderNativeContent` →
  `Window_attachPanelIOSurface`, so `VkPane_register` always finds a live
  device. Metal layers are attached before the composite frame is set, so a
  zero-extent capability report falls back to the registered pixel size —
  never fail registration on a frameless layer.

## 12. IOSurface / Native Pixel Rule
All `IOSurface` allocations and Vulkan renders into them must use **native hardware
pixels**, not logical points.

```c
// CORRECT — multiply point size by the monitor scale factor
int pxW = (int)(rect.z * kx + 0.5f);
int pxH = (int)(rect.w * ky + 0.5f);
renderChildToIOSurface(child, surface, pxW, pxH);

// WRONG — logical points only, blurry on Retina
renderChildToIOSurface(child, surface, (int)rect.z, (int)rect.h);
```

The `CALayer` frame is always set in **logical points** (CoreAnimation convention).
`contentsScale` on an IOSurface-backed layer must be set to `backingScaleFactor` (e.g. `2.0` on Retina)
so CoreAnimation maps the native physical pixels of the pre-rendered `IOSurface` to logical points at
exact 1:1 screen resolution, preventing the content from appearing doubled in size.

## 13. IOSurface Panel Gravity
Each `CALayer` backing an IOSurface panel gets its `contentsGravity` and
`anchorPoint` set from the panel's `selfAnchor`. This keeps the rendered pixel
content pinned to the correct corner during live window resize (before the next
frame is ready). The mapping is:

```
TOP_LEFT     → kCAGravityTopLeft    / anchorPoint (0,0)
TOP_CENTER   → kCAGravityTop        / anchorPoint (0.5,0)
TOP_RIGHT    → kCAGravityTopRight   / anchorPoint (1,0)
MIDDLE_LEFT  → kCAGravityLeft       / anchorPoint (0,0.5)
CENTER       → kCAGravityCenter     / anchorPoint (0.5,0.5)
MIDDLE_RIGHT → kCAGravityRight      / anchorPoint (1,0.5)
BOTTOM_LEFT  → kCAGravityBottomLeft / anchorPoint (0,1)
BOTTOM_CENTER→ kCAGravityBottom     / anchorPoint (0.5,1)
BOTTOM_RIGHT → kCAGravityBottomRight/ anchorPoint (1,1)
```

The `CAMetalLayer` hosting the Vulkan swapchain has `geometryFlipped = YES` so
that Vulkan's top-down coordinate space maps correctly onto CoreAnimation's
bottom-up space. IOSurface `CALayer`s also get `geometryFlipped = YES` for the
same reason.

## 14. No Double-Render Law
When `nativeContent` mode is active (IOSurface children), the Vulkan swapchain
blit loop (`presentFrameTail`) **must skip** all non-scene panels. They are
already composited natively by AppKit. Only `TYPE_SCENE3D_SINGLETON`,
`TYPE_SCENE2D_SINGLETON`, and `TYPE_SCENE_SINGLETON` children may be stamped
onto the swapchain, and **only if they are not pane-backed**: a scene child
holding its own `CAMetalLayer` pane (`PanelCocoa_isMetal`) is composited by
WindowServer and must never be stamped onto the board either — it renders
through `VkPane_presentAll` only. Fixed-size panes never rebuild.

## 15. Commit and Push Discipline
- **Never push unless explicitly asked.** A push request is a one-time button press; do not auto-push subsequent changes.
- **Always commit locally.** Continue implementing granular, per-class local commits regardless of whether a push was requested.
- Do not commit unless the change actually fixes or finishes something — broken
  intermediate states stay dirty on disk.
- Commits are per logical fix/feature per class. Keep classes and features isolated.

## 16. Pointer declaration spacing
Pointer declarators are always `T *name` — one space before `*`, `*` binds to the name, no space after `*`.

```c
Buffer *buf;                          // yes
struct IOSurfaceChild *child;         // yes
void *data;                           // yes
void *Memory_alloc(uint32_t t, size_t n); // yes — return type follows same rule

Buffer* buf;                          // no — star touches type
Buffer * buf;                         // no — space after star
void* data;                           // no
struct IOSurfaceChild* child;         // no
```

Casts are the sole exception: `(T*) var` — no space before `*` inside the cast, then one space after `)` per rule 2:

```c
(void*) ptr;                          // yes
(Block*) (aligned - HEADER_SIZE);     // yes
(SpinLock*) lock;                     // yes
(int64_t*) addr;                      // yes — same for deref casts: *(int64_t*) addr

(void *) ptr;                         // no — space before * inside cast
(void*)ptr;                           // no — missing space after )
(Block *) ptr;                        // no
```

Declarators vs casts: `T *name` in declarations, `(T*) var` in casts — star binds to name in decls, to type inside cast parens. Multi-pointer or function-pointer follows decl spacing: `char **argv` is `char **argv` (space before first `*`), `void (*fn)(void *arg)` etc. When in doubt, `*` stays with name in decls, with type inside `( )` casts.

---

## 17. Vertical Integration — Single Order To Follow (R1 > R2 > R3 > R4 > R5)
The stack has ONE order. Lower R = boots earlier, more stable, tears down later. Follow this everywhere.

```
┌────────────────────────────────────────────────────────────────────────┐
│ R1 — HOST (hotcwap)                                                    │
│ Process supervisor, Kernel, OS windows, dynamic hot-loader, lifecycle │
│ Hot / Manifest / VkLoader / Window (window_cocoa.m) / Application      │
│ boots FIRST, tears down LAST. Window never hot-updates (OS-owned).     │
└──────────────────────────────────┬─────────────────────────────────────┘
     supervises ▼                   │ borrows leaf shape ▲
┌──────────────────────────────────┴─────────────────────────────────────┐
│ R2 — BEHAVIOR (vexspoke)                                               │
│ Relational memory arena, BitPool, Variable, types, dest-last math, sync│
│ Variable / BitPool / Memory / RingBuffer / SpinLock / Type / Vec+Mat   │
│ pure leaf: includes NOTHING above or below. Supervised by R1.          │
└──────────────────────────────────┬─────────────────────────────────────┘
     supervises ▼                   │ borrows shape ▲ (includes R2 only)
┌──────────────────────────────────┴─────────────────────────────────────┐
│ R3 — DRIVER (graphvex, api-haven, language, darkbase)                  │
│ Raw hardware/network/syntax drivers: GPU & WGPU, REST/WS, AST, DB     │
│ Buffer family / Texture / FontBake / shader/spv / REST core / MCP     │
│ no window, no UI tree, no services.                                    │
└──────────────────────────────────┬─────────────────────────────────────┘
     supervises ▼                   │ registers / binds ▲
┌──────────────────────────────────┴─────────────────────────────────────┐
│ R4 — INTERFACES (darling-framework, sesh)                              │
│ Visual & collaboration substrate: 9-grid UI, compositor, session sync  │
│ Panel / Container / Canvas / widgets / IOSurface / event dispatch      │
└──────────────────────────────────┬─────────────────────────────────────┘
     supervises ▼                   │ consumes / instances ▲
┌──────────────────────────────────┴─────────────────────────────────────┐
│ R5 — INTERACTABLES (semicolon, samplerate, darling-editor, drawling, anti)
│ End-user applications: IDE, DAW, Spatial Whiteboard, Painting, 3D Game │
│ Each = Application {CLI/TUI/GUI} x windows[APP_MAX_WINDOWS] in Kernel. │
└────────────────────────────────────────────────────────────────────────┘
│ vexgraph — umbrella integrator (projects/, main/vk_test.c, tooling)    │
└────────────────────────────────────────────────────────────────────────┘
```

How to read the arrows (the only two directions in the whole repo):
- `supervises ▼` (runtime): R1 boots R2, loads R3 dylibs, registers R4 UI, R5 apps. Teardown runs reverse per Rule 26.
- `borrows shape ▲` (compile-time): a file may only `#include` shapes from the allowlist below. Supervisor borrows leaf shapes; leaf never borrows supervisor shapes.
- `registers ◀` (engines): R5 never gets `#include`d by R1. R1 holds `void*` + `AppRunFn`/`AppTickFn`/`AppHotReloadFn` + `HotModule`. Same feature, no circular link, Rule 19 standalone stays green.

Allowlist (only includes permitted — everything else is a defect):
- R2 `vexspoke`: includes NOTHING from `graphvex`/`hotcwap`/`darling`/`api-haven`/engines. Pure leaf.
- R3 `graphvex`: includes `vexspoke` only. Never `hotcwap`/`darling`/`api-haven`/engines.
- R3 `api-haven`: `vexspoke` only, no graphics; may define pure connector contracts — descriptor registries plus fn-pointer client shapes (e.g. `AiProvider`, `DbProvider`, `McpServer` — `AppDetect`/`CaptureTool`/`ProcessProbe` live in vexspoke R2 and are consumed, never re-implemented) — with zero vendor/database includes; contract class names never collide with owner interfaces (the `Database` interface stays with darkbase). MCP tool/resource surfaces (stdio JSON-RPC engines and their `mcp_server` runners) are connector-shape hosting and live in api-haven; they host handler closures over those registries/probes only — no exec, no writes, no vendor SDKs.
- R3 `language`/`darkbase`: `vexspoke` (+ `graphvex` for GPU-backed ones) only, never engines.
- R1 `hotcwap`: includes `vexspoke` (arena/event/input/time) + `graphvex` (buffer/GPU) only. Never `darling`/`api-haven`/database/language/engine headers.
- R4 `darling-framework`: includes `vexspoke` + `graphvex` + `hotcwap` (`window/window.h`) only. Never `api-haven`/engines.
- R4 `sesh`: `vexspoke` + `api-haven` (WsFanout) only. Never graphics engines.
- R5 engines (`semicolon`, `samplerate`, `darling-editor`, `drawling`, `anti`): borrow shapes from R1/R2/R3/R4 to build; own no OS/window/memory management — borrow arenas, windows, GPU instances from R1. Standalone-capable or Kernel-registered.

Managed exceptions — socket/spawn/decode seams (Rule 33, Tier 1 preserved):
- R2 `vexspoke` owns the `WsClient` + `ProcessSpawn` leaf drivers. `WsClient`
  is a bounded frame slot (fixed rx buffer, state, cancel flag, timeoutNs):
  no threads, no owned sockets — the R1 driver owns the socket and feeds
  bytes in; `WsClient_poll` waits at most 100ms in ~1ms cancel-checked
  slices (Rule 27, drop-degrade false). `ProcessSpawn` is a bounded
  child-job table (fixed slots, cancel flag): `posix_spawnp` launch (never
  `system()`), non-blocking `waitpid` reap slices bounded to 100ms, `SIGTERM`
  cancel — no blocking wait, no `UINT64_MAX`, zero threads.
- R3 `graphvex` owns `FrameImporter` (pure RGBA8→`Image` upload via
  `Image_upload`): no `popen`, no `libav*` include/link, no threads; callers
  decode through the `ProcessSpawn` shape and never call it from
  tick/render paths.
- R3 `api-haven` owns `HavenWsFanout` (16-slot fan-out registry over opaque
  `void*` handles + `WsSource` fn-table `{connect, poll, send, close}`,
  `pollStep` budget-driven by R1) + `AiSse` (pure caller-fed incremental
  SSE `data:`/`event:` state machine bound explicitly to one fanout slot
  via the existing `WsSource` table, R1-budgeted `pollStep`, 100ms slices,
  drop-degrade false, zero socket/thread/alloc, truncation flag per
  Rule 35.3) + `DbProvider` catalog-only read-only SQLite file row
  (`DbSqliteFile`: path, caps, bounds, fn-table only, zero `sqlite3.h`
  include/link — execution delegates via opaque handle to the database
  owner / vexspoke File/VFS) + `AssetBroker`
  (`AssetBroker_downloadToCache` bounded chunked-copy
  `(srcChunk, dest, destCap, outTruncated)` dest-last, per-chunk 100ms
  budget + cancel flag, never a whole-file budget, `VexHome_cache`-confined,
  closed before `Memory_freeAll`): zero `pthread_*`, zero socket syscalls,
  no scraping, no exec, no vendor SDK; transports stay in R2, driven by
  R1 callbacks only.

Build/commit order (dependencies first, per Rule 20): `vexspoke` -> `graphvex`/`api-haven`/`language`/`darkbase` -> `hotcwap` -> `darling-framework`/`sesh` -> `semicolon`/`samplerate`/`darling-editor`/`drawling`/`anti`. Boot order is the reverse crown: R1 first.

1. **R2 `vexspoke` Behavior**:
   - Owns: `Variable`, `BitPool`, `Memory`/`MemoryArena`, `RingBuffer`/`SpinLock`, `Type`/`Class`, math, `http`/`json`, `VexHome`/`File`/`Log`, audio, base Vulkan context, **system capability probes (`AppDetect`, `CaptureTool`, `ProcessProbe`)**.

2. **R3 Drivers — graphvex | api-haven | language | darkbase**:
   - `graphvex`: `spv/` blobs, `Buffer` family, `Font`/`FontBake`, `SdfGpu`, `Texture`, `Raster`, `WgpuBackend`.
   - `api-haven`: API surface, telemetry, webhooks + connector contracts (AI providers, app detection, database catalog/connector shapes) + the MCP tool/resource server (`McpServer`, `mcp_server` stdio runner) hosting them.
   - `language`: `Language` contract (`Lang_tokenize/parse/highlight/...`), each grammar a hot-swappable dylib.
   - `darkbase`: `Database` interface, native vex store in-budget.

3. **R1 `hotcwap` Host**:
   - Owns: `Kernel` (`projects/hotcwap/kernel/kernel.h`), `Application` registry (`app/application.h`), `Window`, `Hot`/`Manifest`/`VkLoader`/`SpvWatch`.
   - `Application` is final infrastructure — R5 apps rely on it, it never relies on them.

4. **R4 Interfaces — darling-framework | sesh**:
   - `darling-framework`: `Canvas`/`Container`/`Panel`/widgets/compositor/`panel_bridge.c`.
   - `sesh`: Session sync, VPS relay, Cloudflare edge, in-engine bug ingestion.

5. **R5 Interactables**:
   - `semicolon`: Mini IDE (5MB jGRASP/Zed target, shell-out toolchains, xlsx viewer).
   - `samplerate`: Bare-metal DAW (realtime mixer, spatial audio, 3D HRTF).
   - `darling-editor`: Spatial studio (Figma + Miro board, infinite canvas, HTML/SVG export).
   - `drawling`: Drawing studio (GIMP/Krita/FlipaClip target, layers, brushes).
   - `anti`: 3D game engine (5-column editor, bindless, meshlets, physics, darkbase).
   - N apps x M windows per `Kernel`.

6. **`vexgraph` (Top-Level Integrator & Application Root)**:
   - The umbrella project that nests the repositories in `projects/` and builds unified binaries, probes (`main/vk_test.c`), and tooling.

## 18. Canonical Include Paths (Zero Parent Hops)
Headers must **never** traverse upwards with `../` or `../../` to cross module or repository boundaries. Every `#include` must be rooted at the canonical subsystem directory.

```c
#include "nio/mem.h"               // yes
#include "../nio/mem.h"            // no — legacy monorepo artifact

#include "vulkan/vk.h"             // yes
#include "../vulkan/vk.h"          // no

#include "darling/panel.h"         // yes
#include "font/font.h"             // yes
#include "../font/font.h"          // no

#include "window/window.h"         // yes
#include "hot/hot.h"               // yes
#include "io/vfs.h"                // yes
#include "../../io/vfs.h"          // no
```

This ensures that every source file compiles identically whether it is built inside the unified `vexgraph` tree or as a standalone repository via `FetchContent`.

## 19. Standalone Autonomy & Target Seam Law
Every repository (`vexspoke`, `graphvex`, `hotcwap`, `darling`, `api-haven`) must remain buildable both **standalone** and **in-tree** inside `vexgraph`.

Downstream repositories must guard their upstream dependencies with the `if(NOT TARGET ...)` target seam:

```cmake
# Example in darling/CMakeLists.txt or hotcwap/CMakeLists.txt:
if(NOT TARGET vexspoke)
    # Standalone build: pull upstream via FetchContent
    include(FetchContent)
    FetchContent_Declare(
        vexspoke
        GIT_REPOSITORY https://github.com/vexgraph-dev/vexspoke.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(vexspoke)
endif()
```

When building inside `vexgraph`, `vexspoke` already exists as an in-tree target. The guard prevents duplicate target definitions and avoids redundant network fetches.

## 20. Multi-Repo Atomic Commit Discipline (Per Feature, Per Subsystem, Per Repo)
Rule 6 (Cohesive Commits: Per Feature, Per Subsystem/Class, Per Repository) and Rule 7 (No Auto-Pushing) apply across all repositories:
- **Per-Repository Execution**: When a change touches a class or feature within a repo, commit locally inside that repository's git root (`projects/<repo>`). Never commit from the umbrella root for sub-repository changes.
- **Atomic Subsystem Isolation**: Keep commits focused to a single class or cohesive subsystem unit (e.g., `feat(cursor): ...`, `feat(text_core): ...`, or `feat(sync): ...`). Classes that operate as a cohesive pipeline land together with their build wiring; unrelated subsystems must never be bundled into a shared omnibus blob.
- **Upstream First**: Cross-cutting changes spanning multiple repositories must commit in strict downward-only dependency order:
  `vexspoke` -> `graphvex`/`api-haven`/`language`/`darkbase` -> `hotcwap` -> `darling-framework`/`sesh` -> R5 apps.

The old order (`vexspoke` -> `graphvex` -> `hotcwap` -> `darling` -> `api-haven` -> `vexgraph`) is retired; `darling` is now a symlink to `darling-framework`.
- **Zero Giant Blobs**: Assessors evaluate each repository's commit history independently. Grouping unrelated subsystems or multiple repos destroys reviewability.
- **Never Auto-Push**: Rule 7 remains absolute. Commit locally, never push unless explicitly requested.
Upstream-first ordering applies to file-pair commits; each repo-local commit is one file pair per above.


## 21. SPIR-V Shader Partitioning & Deployment
SPIR-V shaders (`.spv`) are centralized under `projects/graphvex/shader/` — the single source of truth, laid out by stage:
- `shader/frag/`, `shader/vert/`, `shader/comp/` — GLSL sources (base: `hello_triangle`, `solid_quad`; UI: `texture_quad`, `text_sdf`; compute: `sdf_jfa`, `sdf_combine`).
- `shader/spv/` — compiled blobs (`<name>_<stage>.spv`, bare `<name>.spv` for compute), rebuilt via `shader/build_shaders.sh` (requires `glslangValidator`).
- (Legacy note: sources lived in `hotcwap/vulkan/shaders/` + `darling/vulkan/shaders/`, blobs in per-subsystem `spv/` mirrors — all stale, pending deletion.)
- **`vexspoke` / `darling`**: Own zero blobs; they load via the resolution protocol below. Per-subsystem `spv/` directories, if still present, are stale mirrors pending deletion.

**Runtime Shader Resolution Protocol**:
The runtime loader (`loadSpvAny`) must search in this exact precedence order:
1. `ANTI_SPV_DIR` / `VEX_SPV_DIR` (build-time staging directory `${CMAKE_BINARY_DIR}/spv/`, populated from `projects/graphvex/shader/spv/`)
2. `<exe_dir>/spv/<name>` (adjacent deployment)
3. `<exe_dir>/../Resources/spv/<name>` (macOS `.app` bundle)
4. CWD-relative paths (`spv/<name>`, `src/vulkan/spv/<name>`)

The top-level `vexgraph` CMake build staging copies all `.spv` blobs from `projects/graphvex/shader/spv/` into `${CMAKE_BINARY_DIR}/spv/` so all subsystems discover their shaders seamlessly.

## 22. Identity & Naming Transition (Anti → Vexspoke / VexHome)
The codebase is actively transitioning from the initial `anti` prototype name to the permanent **`vex`** family identity:
- Engine core: `anti` → `vexspoke` (the central spoke of the graph).
- Engine home directory: `AntiHome` → `VexHome`. Canonical per-platform root (created by `VexHome_ensure()`; `VexHome_cache(subsystem)` builds `<root>/cache/<subsystem>/` with `dictionary.ini` via `VexHome_cacheEnsure`): macOS `~/Library/Application Support/vexgraph`; Linux `$XDG_DATA_HOME/vexgraph` (≈ `~/.local/share/vexgraph`); Windows `%LOCALAPPDATA%\vexgraph`; fallback `$HOME/vex` when the canonical base is unavailable. `$VEX_HOME`, when set and non-empty, overrides all of the above (test seam). Never delete or migrate legacy `~/anti` or `~/vex` automatically.
- Preprocessor definitions: prefer `VEX_*` alongside backwards-compatible `ANTI_*` defines (e.g., `ANTI_SPV_DIR` / `VEX_SPV_DIR`).
- Executable names: `vexspoke_demo` (formerly `anti`) is the headless demo harness inside `vexspoke`, while `vk_test` and full applications live in `vexgraph`.

## 23. The `;;OVERVIEW` Documentation & File Layout Standard
Every `.c` (and `.m` where applicable) must be self-contained so that a developer can understand the class, its memory layout, and all its capabilities from the **first 100–150 lines** of the implementation file without having to tab back and forth to the `.h` file.

Constructors are not generic functions—they are arity-overloaded instance initializers (`Class_0()`, `Class_1()`) called via `Class(...)` macros. Functions are therefore strictly partitioned into four categories:
1. **`constructor`** (Instantiation & lifecycle via `CONSTRUCTOR_DISPATCH`)
2. **`core functions`** (Computational logic, rendering, layout, and operational algorithms)
3. **`setters`** (Mutators: `setX(ptr, x)`, `setSize(ptr, w, h)`, `setImage(ptr, img)`)
4. **`getters`** (Accessors: `getX(ptr)`, `getSize(ptr, &w, &h)`, `getImage(ptr)`)

### Required Header Structure (Single CLASS — No MODULE):
```c
#include "annotation/overview.h"
#include "subsystem/class.h"
// ...

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: ClassName (inherits BaseClass -> GrandParentClass)
 * LEVEL: L2 — Behavior (Rule 28: L1 metadata / L2 behavior / L3 module / L4 self-mgmt)
 * ============================================================================
 * Architectural overview of the component, its memory role, and lifecycle.
 *
 * STRUCT FIELDS (Mirroring subsystem/class.h — exactly this file's class):
 * ----------------------------------------------------------------------------
 *   FieldType1 field1;       // Detailed description & alignment/units
 *   FieldType2 field2;       // Detailed description & constraints
 *
 * PRIVATE HELPERS (kept file-local pure-data only, each with full fields):
 * ----------------------------------------------------------------------------
 *   HelperName helper_field1;  // type + name + role per field
 *   HelperName helper_field2;
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - ClassName()                            : ClassName_0()
 *   - ClassName(arg1)                        : ClassName_1(arg1)
 *
 * Core Functions:
 *   - ClassName_process(self, input, dest)   : Primary operational logic
 *
 * Setters:
 *   - ClassName_setX(self, x)
 *   - ClassName_setSize(self, w, h)
 *
 * Getters:
 *   - ClassName_getX(const self)
 *   - ClassName_getSize(const self, outW, outH)
 * ============================================================================
 */
```

### Source Body Organization:
The implementation body must be grouped under distinct visual comment banners in strict order:
1. **`// CONSTRUCTORS`**: `Class_0()`, `Class_1(...)`, and lifecycle instantiators.
2. **`// CORE FUNCTIONS`**: Compute, tick, render handlers, layout algorithms, transformation logic.
3. **`// SETTERS`**: All state mutators (`Class_set*(...)`).
4. **`// GETTERS`**: All field inspectors and state accessors (`Class_get*(...)`).

### The Living Overview Law (Zero Drift):
Any modification, refactor, or addition that touches a struct's fields, constructors, or methods **must update the `;;OVERVIEW` header block in the same commit**. An out-of-date overview is a compiler/code defect.

`MODULE:` headers are banned except for true procedural entry points
(`main/*.c`, `tests/*.c`, thin re-export shims) that own zero structs.
Every class file uses `CLASS:`. A file owning two public structs must be
split before its overview is written — never document two classes under
one `MODULE:` to hide the violation. Private file-local helpers (no API)
are listed under a `PRIVATE HELPERS` section, never as a second `CLASS:`.

---

## 24. Symmetric Getter & Setter Completeness (Java-Library Standard)
In `darling` and high-level class abstractions, every state-bearing field on a class struct must provide complete, symmetric getters and setters, exactly like an idiomatic Java or C# library.

A consumer of the library should never have to manually pierce struct internals or violate the two-layer access cap just to inspect simple state:
- If a `Label` has a `char *text`, it must provide `Label_setText(lbl, text)` and `const char *Label_getText(const Label *lbl)`.
- If a `Label` has `fontSize`, it must provide `Label_setFontSize(lbl, size)` and `float Label_getFontSize(const Label *lbl)`.

### Signature Conventions:
1. **Mutators**: `void Class_set<Prop>(Class *self, <Type> val)`
2. **Scalar / Pointer Accessors**: `<Type> Class_get<Prop>(const Class *self)`
3. **Boolean Accessors**: `bool Class_is<Prop>(const Class *self)` or `bool Class_has<Prop>(const Class *self)`
4. **Multi-Value Accessors**: Follow Rule 9 (Dest Last):
   ```c
   void Class_getSize(const Class *self, float *outW, float *outH);
   void Class_getCrop(const Picture *self, float *outX1, float *outY1, float *outX2, float *outY2);
   ```
5. **Null-Safety**: All getters must defensively check if `self` is `nullptr` and return safe defaults (`nullptr`, `0`, `false`, `0.0f`).

---

## 25. Per-Repo Commit Message Scope (No Repo Prefix — Scope to Class/Subsystem)
A commit lives inside exactly one repository, so the message must read as if
that repository is the whole world. Never prefix with the repo name.

```text
feat(window): add WindowCursorType and cursor selection API   // yes (in hotcwap)
feat(cursor): add Cursor class and predefined singletons      // yes (in darling)
feat(paint): add stroke, base brush, and SDF raster tips      // yes (in graphvex)
feat(mesh): add mesh buffers and 128-triangle meshlet cluster // yes (in graphvex)
test(label): add headless test suite for label and cursor     // yes (in darling)

feat(hotcwap): add WindowCursorType ...                       // no — redundant repo scope
feat(darling): add Cursor, Label, and Compositor ...          // no — unrelated multi-subsystem blob
```

The scope in parens names the *class, subsystem, or seam* within the repo
(`window`, `cursor`, `label`, `mesh`, `paint`, `sync`, `compositor`), following Rule 6:
**one logical feature or cohesive subsystem per repository**. Cross-cutting changes still commit
upstream-first per Rule 20, each with its own repo-local message.
Scope names the class/subsystem; the unit shipped is its file pair (`.h+.c`).

---

## 26. Teardown Order (Destroy Top-Down, Free Last)
Shutdown runs the stack in reverse, and `Memory_freeAll` is always the final
step — never earlier. Shims allocate outside the slabs (`calloc`,
`IOSurfaceCreate`, `strdup`), so freeing the arena first orphans every
surface, layer, and native handle with no record of the leak.

```
Window_destroy(w)            // detach adapters, close (never release-then-use)
  → Darling_shutdownCompositor()  // per-panel surfaces + layers first
    → Vk_shutdown()               // device, surface, instance
      → Thread_stopAll() / joins  // workers joined BEFORE their resources die
        → Memory_freeAll()        // arena reset LAST, after all teardowns
```

- Detach before free (Lesson 9): registry → parent → free. `PanelCocoa_free`
  must run before its `Panel` is freed; a `Window` detaches adapters before
  `close`.
- A skipped step is a leak, not a shortcut. If a probe exits without
  `Window_destroy`, the `NSWindow` outlives the process as a ghost.
- Multi-app Kernel order (`R1` host, N apps x M windows): `Kernel_destroy`
  stops all `Application`s (`running = false`) top-down (R5 → R4 → R3),
  destroys darling nodes per-panel, closes all windows, bounded-joins present/worker
  threads per Rule 27, shuts down Vulkan, resets `transientArena`, destroys master
  `arena` LAST. Never free `arena` while any `Application`/`Window` still runs.

---

## 27. No Unbounded Waits on Joined Threads (Bound Every Block)
Any thread another thread joins must reach its exit check within a bounded
time on every path. An infinite wait (`UINT64_MAX` fence, endless queue poll)
on a worker parks `pthread_join` forever and freezes teardown — the window
goes ghost, unstopped threads keep logging, `Window_destroy` never runs.

```c
// no — hangs teardown when the drawable dies (fullscreen close)
WaitForFences_fn(dev, 1, &fence, VK_TRUE, UINT64_MAX);

// yes — drop the frame, re-check running, join succeeds in ~100ms
if (WaitForFences_fn(dev, 1, &fence, VK_TRUE, 100000000ULL) != VK_SUCCESS)
    return false;
```

- Fences, queue waits, and device idles on worker paths all take explicit
  timeouts with a drop-degrade path (skip frame, keep old content, return).
- The bound is generous (100ms ≈ 6 frames) so healthy hitches never trip it;
  only a dead resource does. A timeout firing in the log means the resource
  died — investigate the resource, not the timeout.

---

## 28. Four System Levels L1–L4 (File Stability — NOT Runtime Rank R1–R5)
Every file lives on exactly one L level. Stability increases downward; replaceability increases upward. `L` answers "how safe is it to edit this file?" `R` (Rule 17) answers "who boots/supervises whom at runtime?" Never mix them: `R1 hotcwap > R2 vexspoke > R3 graphvex/api-haven > R4 interfaces > R5 engines` is supervision; `L1–L4` below is edit-risk. A `Kernel` file is `LEVEL: L4` living at `R1` — write `LEVEL: L4`, never `LEVEL: R1`.

```
L1  FILE METADATA ............ declarative, easily replaced custom stuff —
                                the files other files consume: manifests,
                                descriptors, schemas (hot/manifest.h, JSON).
                                Swappable with zero code changes.
         │ depends on
         ▼
L2  BEHAVIOR ................. how custom structs/classes work: the class
                                API surface — constructors, core functions,
                                setters, getters (Rule 23 registries).
         │ depends on
         ▼
L3  MODULE CODE .............. actual .dylib behavior: business logic running
                                inside reloaded modules. Never window, OS, or
                                memory management (hot/hot_behavior.c).
         │ depends on
         ▼
L4  SELF-MANAGEMENT .......... the bottom that manages everything above:
                                hotcwap's loader (watch, verify, swap, retire),
                                the OS window, memory management. The system
                                reflecting on itself — fixing and upgrading
                                itself (hot/hot.c, hot_trampoline.c,
                                hot_retire.c, window/, nio/mem.c).
```

- A file's `;;OVERVIEW` must declare its level: `LEVEL: L1 — File Metadata`
  (and so on) directly under the `CLASS:` line, so a reader knows instantly
  how stable vs replaceable the file is.
- L-levels (file replaceability) are orthogonal to R-levels (Rule 17 runtime supervision R1–R5). A `Kernel` file is `LEVEL: L4 — Self-Management` living at `R1 Host`. Never write `LEVEL: R1` — levels are L, supervision is R.
- L4 files change rarely and review heavily: a bug at the bottom breaks
  every level above. L1 files change freely: a bad manifest only breaks
  one module load, caught by ABI verification before any swap.
- New files default to the highest level they can live on. Pushing logic
  downward (L2 → L4) needs justification; it makes the foundation bigger.

---

## 29. Sub-Part Field Segregation & the `Class_part_verb` Law
A widget that owns a sub-object (caret, scrollbar, gutter, thumb) exposes it
only through `Class_part_*` verbs — never by piercing `->field`. The struct
in the header must segregate fields under part banners so a reader sees at a
glance which state is the owner's and which belongs to each part:

```c
typedef struct Input {
    // --- Input core (owner fields: text state, not any part) ---
    Panel base;
    char *text;
    ...
    // --- Caret part (field->caret->verb; views only, never pierce) ---
    int caretMode;
    uint32_t caretColor;
    ...
    Panel *caretView;   // borrowed visual (null = default); detach-only
    ...
} Input;
```

- Owner core first, then one banner per part, in the order the parts are
  documented in the `;;OVERVIEW`. A part that forwards to another node
  (e.g. ScrollContainer's `panel_*` over content) owns NO fields — say so in a
  `NOTE:` line, since new stored state there is a design smell.
- Sub-object pointers are.views.: borrowed, detach-only, never freed or
  reparented by the owner. Replacing a view (`scrollbar_setBar`,
  `caret_setView`) detaches the old one and attaches the new one; the arena
  owns the memory, the owner owns the relationship.
- The `;;OVERVIEW` STRUCT FIELDS section mirrors the same banners verbatim
  (Rule 23 living blueprint) so header and source can never drift apart.
- Every part field keeps symmetric getters/setters per Rule 24 — the part
  API is ergonomic precisely so nobody reaches for `->`.

**Container-vs-Panel law:** a Container is a multi-child layer-owner (owns N child
layers, manages attach/detach/surfaces, Rule 29 parts); a Panel is a single-surface
leaf painter (one IOSurface, no child layer management). Multi-child managers are
named *Container and embed Panel as first member; leaf painters are named *Panel
or own widget class. TabbedContainer is banned — tabs are a mode of SectionContainer.

---

## 30. Living Darling Docs (Zero Drift Between Code and `_docs/darling.md`)
`_docs/darling.md` (1699+ lines: every widget field, compartment, function,
getter/setter, plus compositor / WindowServer / IOSurface / Vulkan) is a
load-bearing artifact, not a snapshot. An out-of-date section is a defect,
same as a stale `;;OVERVIEW` under Rule 23.

- **Scope — every class root in darling.** Each class struct / file pair under
  `projects/darling/` is a root: `Container`, `Panel`, `Canvas`, every widget
  (`ListContainer`, `GridContainer`, `ScrollContainer`, `SectionContainer`, `LayeredContainer`, `SplitContainer`,
  `MarkdownPanel`, `RichTextPanel`, `Button`, `Switch`, `Checkbox`,
  `RadioGroup`, `Slider`, `Knob`, `Input`, `Textarea`, `InputOTP`, `Select`,
  `DatePicker`, `ColorPicker`, `ColorSwatch`, `ScrollBar`, `Label`,
  `RichLabel`, `Typography`, `Kbd`, `Dialog`, `AlertDialog`, `FileDialog`,
  `ColorDialog`, `Picture`, `Plot`, `Scene`/`Scene2D`/`Scene3D`, `RichText`),
  plus the substrate (`darling-type.h`, compositor, events/dispatch/bridge,
  anim, raster/surface, io/mmap/VFS/fontbake, ObjC bridges, shaders/SPV).
- **Triggers — any of these must update the docs:** struct field
  added/removed/renamed/retyped/default-changed; constructor/core/setter/getter
  signature or behavior change; layout algorithm, clamp, dirty-flag, or
  ownership (`owned` vs `borrowed`) change; compartment move (e.g. a field
  changing from `style` to `state`, a new `feel` or `bridge` slot); compositor /
  IOSurface / Vulkan / WindowServer / event / anim contract change; new or
  removed widget.
- **Same-commit law (extends Rules 6, 23).** Code + `;;OVERVIEW` header +
  the matching `_docs/darling.md` section land in the SAME granular per-class
  commit. Never a follow-up "update docs" commit — follow-ups never happen
  at 2am. A commit that changes darling behavior without its docs section is
  a broken intermediate state per Rule 15: keep it dirty on disk, do not commit.
- **What "updated" means.** Field table row (type, default, compartment, role);
  function entry with exact C signature + side effects (dirty? layout? clamp?
  reparent?); compartment label corrected if the field moved; one `why`
  sentence if the rationale changed; TOC entry if a section is added/renamed.
  If 2am-you cannot reconstruct the behavior from the docs section alone, the
  update was incomplete — say so in the commit message and finish it.
- **Reviewer checklist (every darling commit):**
  1. `;;OVERVIEW` STRUCT FIELDS + FUNCTION REGISTRY mirror the new struct/API?
  2. `_docs/darling.md` section mirrors the new fields/functions/compartments?
  3. Stub-vs-live status corrected (`;;INCOMPLETE` gained or retired)?
  4. Backend sections (§41–§48) touched if pixels, events, or teardown changed?
Same-commit law is per file pair: code pair plus overview plus the matching `_docs/darling.md` section land together; splitting them across commits is a broken intermediate state.

---

## 31. The AI-First Architecture Manifesto & Boilerplate Defense
### Definition:
The extreme, verbose, and "masochistic" boilerplate spanning this codebase (zero arrow sugar `(*ptr).field`, strict single-class-per-file Java Law, explicit C constructor overloads, vtable dispatches, symmetric getters/setters, dest-last parameters, two-layer member dereference caps, and zero steady-state allocation) is NOT an accident, nor a misunderstanding of idiomatic C. It is an intentional, rigorous architectural manifesto of **AI-Human Pair Systems Programming**.

### The Why:
1. **Machine Comprehension & Context Density**:
   Modern LLMs and AI coding agents operate with maximum precision and zero hallucination when code contracts are explicit, typed, un-aliased, and local. Arrow sugar `->` obscures the pointer dereference boundary; implicit constructors hide initialization order; packing multiple classes into a single file pollutes the model's context window and causes cross-struct hallucinations. By restricting every file to a single class struct and its explicit methods, an AI agent can hold the complete, un-truncated operational reality of any class in its active reasoning window.
2. **Deterministic Mechanical Syntax (`(*ptr).field`)**:
   In C, `.` denotes value access and `->` denotes pointer dereference. By writing `(*ptr).field`, the dereference is visually and syntactically explicit. The developer and the AI agent are constantly reminded of memory hops, preventing casual indirection and cache-line thrashing.
3. **The AI Agent as Boilerplate Engine**:
   Human developers historically embraced macro trickery, implicit casting, and sloppy multi-class dumping to avoid typing repetitive boilerplate. In this project, **an AI coding agent writes, verifies, refactors, and maintains the dense boilerplate**. The human software architect directs high-level architectural invariants, algorithms, and concurrency semantics, while the AI agent reliably stamps out the explicit getters, setters, overviews, and constructor dispatches. The boilerplate is no longer a human typing burden—it is a machine-readable safety scaffold.

### The Sanity Warning:
> [!WARNING]
> **SANITY NOTICE FOR EXTERNAL CONTRIBUTORS**
> This repository is not designed for traditional C conveniences, casual hacking, or stylistic shortcuts. It is an unapologetic, machine-verifiable manifesto of AI-augmented systems architecture.
>
> **If you do not approve of this architecture or cannot find peace with this philosophy, consider leaving this repository for your own sanity.**
>
> We do not accept Pull Requests, issues, or stylistic refactors attempting to re-introduce `->`, collapse multiple classes into single files, eliminate explicit getters/setters, or "modernize" the code against our architectural doctrine. The upstream codebase is exclusively maintained by its author in tandem with the AI agent.

---

## 32. Living Preferences Law (Zero Drift for Invariants)
### Definition:
`preferences.md` at the root of the `vexgraph` workspace is the supreme constitutional law and single source of truth for the entire multi-repo ecosystem (`hotcwap`, `darling`, `vexspoke`, `graphvex`, `api-haven`).

### The Why:
In a multi-repository workspace consisting of independently versioned C and native libraries, architectural entropy and convention drift are fatal. If rules live only in developer memory, chat histories, or scattered READMEs, rules will be contradicted and broken within days. A system with zero GC and manual memory layouts requires absolute, unbroken alignment across all subsystems.

### The Rule:
1. **Same-Cycle Update & Local Commit**:
   Whenever an architectural invariant, convention, rule, or preference is introduced, modified, refined, or clarified, `preferences.md` must be updated and locally committed in the same development cycle. Out-of-date preferences are an architectural defect.
2. **Universal Reference Link**:
   Every sub-repository must include a `CONTRIBUTING.md` that explicitly links back to `vexgraph/preferences.md` as its supreme guiding authority.
3. **Subsystem Conformance**:
   Every implementation across `hotcwap`, `darling`, `vexspoke`, `graphvex`, and `api-haven` must adhere strictly to the rules codified herein. No repository is exempt.

---
 
## 33. Conflict Triage — Managed Exception, Not Veto
### Definition:
When rules conflict, or intent outgrows a rule, the answer is never a bare "this violates X." It is "unless you want it, here is how we manage it." The thought prevails; the rules adapt in the same cycle per Rule 32.
 
### The Why:
A veto-only system freezes ambition (multi-app Kernel, R1–R5 ecosystem, 30 grammars, game engines). Tier 1 exists to prevent crashes, not to prevent thinking. Every conflict is triaged, given a managed path, and codified so the next agent inherits the decision.
 
### The Protocol:
1. **Name the tiers:** Tier 1 (crash/leak/deadlock/memory/thread safety) beats Tier 2 (model/contracts) beats Tier 3 (syntax). State which tier each conflicting rule lives on.
2. **Assess before blocking:** state applicability first — does the rule actually cover this case (link-time vs runtime, single-app vs Kernel multi-app, global vs per-arena)? A misapplied rule is not a violation.
3. **Managed exception:** propose the indirection that preserves Tier 1 while granting intent. Canonical moves: opaque handle + callbacks instead of downstream `#include` (keeps Rule 17/19); fixed array + count + getter instead of `**` chains (keeps Rule 10); `MemoryArena_create/freeAll` + bounded-join instead of globals (keeps Rules 26/27); `;;INTENTION("reason")` + `;;DRAFT` markers for Tier 2/3 waivers.
4. **Prefs patch in-cycle:** if intent prevails, draft the exact `preferences.md` wording change now. Tier 1 waivers additionally require an alternate safety proof (no unbounded wait, no use-after-free, no circular link) reviewed heavily. Tier 2/3 waivers require `;;INTENTION` + overview/docs update in the same commit.
5. **Never silent drift:** a managed exception without its prefs + overview + docs update is a defect, same as stale prefs under Rule 32.
 
---
 
## 34. Asset Sourcing — Legal-Sense, First-Class In-App Marketplace Policy
### Definition:
Every external asset source (images, 3D, textures, audio) is a row in api-haven's AssetProvider descriptor registry. Only blessed public APIs and direct-download URLs offered by the source are wired. Sources without a public search API are catalog-only rows with curated static manifests and hand-verified URLs, or excluded. Interface scraping — parsing another service's HTML/JSON to fake a search API, or bypassing auth — is a defect, always.
 
### The Why:
Legal exposure, broken trust, and brittle integrations come from scraping. A first-class in-app marketplace must be built on explicit contracts, normalized shapes, and license-aware flows — not on reverse-engineered endpoints that vanish or change without notice.
 
### The Rule:
- **Catalog, not scraping.** Blessed providers: Unsplash, Pexels, Pixabay, Openverse, Wikimedia Commons, Sketchfab, Freesound, Poly Haven, AmbientCG, OpenGameArt, Google Custom Search JSON API. Each is a row in the AssetProvider registry with its public search API. Sources without a public search API (Pinterest, raw Google Images, Kenney, Quaternius, itch.io packs) are catalog-only rows with curated static manifests and hand-verified URLs, or excluded. Interface scraping is a defect.
- **One normalized contract.** Every search result is an AssetRow (provider slug, id, title, author, license family, preview/download URLs, attribution, dimensions/duration, size). The UI never sees provider-specific shapes.
- **License is a field, not a footnote.** Every row carries a license family; attribution is rendered before import; project export fails closed on UNKNOWN license.
- **Downloads land in the cache.** AssetBroker_download streams into VexHome_cache(<subsystem>) with bounded timeouts (Rule 27); cache files are shim state tracked and closed before Memory_freeAll (Rule 26). No exec, no writes outside the cache.
- **The UI seam is fn-pointers.** darling hosts AssetBrowser and never includes api-haven (Rule 17); the R5 app binds an AssetSource fn-pointer table (opaque handle + callbacks — the Rule 33 canonical move).
- **MCP surface.** asset_source_lookup / asset_search / asset_download hosted by McpServer; writes cache-confined, timeouts bounded, no exec.
- **Credentials.** API keys via vexspoke Keychain or ASSET_KEY_<SLUG> env rendered by ApiAuth; never stored in the arena, prefs, or repo.

---

## 35. Cold-Strict, Hot-Minimal Validation (Crash-Guard Split)
### Definition:
Validation splits by path temperature. The Tier-1 crash-guard half: no function
ever crashes, blocks unboundedly, allocates, or use-after-frees on null,
out-of-bounds, overflow, cancelled, or timed-out input — it returns `false` or
a Rule 24 safe default instead. The Tier-2 contract half: setters validate at
least as strictly as getters, with the reject-or-clamp policy stated in the
`;;OVERVIEW`; getters return safe defaults per Rule 24.

### The Why:
Unvalidated cold input (network bytes, JSON, spawned output, checksums) is how
null dereferences and overflows enter the system; re-validating every element
on a 60fps hot path is how frames die. Validate once where input enters, trust
the validated handle where pixels move. A silent truncation or an unlogged
cold drop corrupts state; a log line per hot frame corrupts performance.

### The Rule:
1. **Cold paths validate exhaustively, once.** R1-entry / R2-boundary seams
   (`AiProvider_get`, `ApiAuth_apply`, `Rest_postJson`,
   `McpServer_handleLine`, `HavenWsFanout_pollStep`, `ProcessSpawn_spawn`,
   checksums) check every hostile input: null, empty, wrong-id, bounds,
   integer overflow, cancelled, timeout. One `Log_warn` per failure at most,
   then drop-degrade per Rule 27 (return `false`, keep old content, move on).
2. **Hot paths guard minimally, never log.** Vk present, `Raster`, `SdfGpu`,
   darling layout, `Kernel_tick`, `presentFrameLocked`: at most one `nullptr`
   entry guard returning `false`, zero per-element revalidation, zero logging,
   zero allocation. The hot path trusts the cold-validated handle. Deeper
   invariants are proven at compile time (`_Static_assert`) or declared as
   `;;INTENTION("reason")` + `;;DRAFT` per Rule 33, never re-checked per frame.
   ```c
   if (self == nullptr)
       return false;
   ```
3. **Truncation is never silent.** Copying into a bounded buffer takes
   `(src, dest, destCap, outTruncated)` — dest-last per Rule 9, flag last.
   On cut: return `false` and set the flag so the caller degrades loudly.
   ```c
   bool Class_copyText(const char *src, char *dest, size_t destCap, bool *outTruncated);
   ```
4. **Seam tests at cold boundaries only.** The full
   nullptr / empty / wrong-id / overflow / truncation / cancelled / timeout
   matrix lives at public cold seams in `tests/*.c` `MODULE` harnesses. Hot
   paths carry `nullptr`-guard-only tests — no per-element matrix, no timing
   harness on the frame path.

---

## 36. Data-Oriented Storage, Object-Oriented Ergonomics
### Definition:
Collection patterns (nodes, lists, tables, trees) use data-oriented storage — flat
arrays, index-based relationships, zero pointer chasing — with object-oriented
ergonomic API: class methods, part verbs (Rule 29), symmetric getters/setters
(Rule 24), dest-last parameters (Rule 9). This is the default for any
collection of records.

### The Why:
Index-based flat arrays keep CPU cache lines hot and make bulk traversal
mechanically simple; object-oriented ergonomics (methods, getters/setters,
dest-last) make the resulting API feel familiar to anyone trained in Java or C#,
without sacrificing hardware-level performance. The two are not in tension —
they are complementary halves of a modern C23 collection design.

### The Rule:
1. **Storage is flat and index-based.** Parent/child relationships are encoded
   as integer indices into a flat array, never as pointer-chased linked lists.
   Pre-order array layout is the canonical form for trees: a node's entire
   subtree occupies a contiguous range `[nodeIndex, nodeIndex + subtreeSize)`,
   so bulk traversal never jumps.
2. **API is object-oriented.** Every collection is a class (Rule 3) with
   constructors, part verbs (Rule 29), symmetric getters/setters (Rule 24), and
   dest-last output parameters (Rule 9). Consumers never pierce internals.
3. **`ExpandableListContainer` + `ExpandableNode` is the canonical instance**
   for hierarchical/nested lists: file trees, task checklists, outlines, mind
   maps, schema viewers. `ExpandableNode` is a behaviourless slot record
   (Rule 3 SLOT RECORD) owned by `ExpandableListContainer`.
4. **Checklists are not a separate class.** A checklist is an
   `ExpandableListContainer` instance with `checklistMode` enabled — never a
   distinct `Checklist` or `TaskList` type. The checkbox lives at child slot 1
   within the row panel, toggled via `setChecked`/`isChecked`.
5. **FileList is composition, not a separate class.** A file tree viewer
   composes `ExpandableListContainer` with R2 `File`/`VFS` data — never a
   standalone `FileList` class. The data is R2; the widget is R4 darling.