# vexgraph's vexspoke — C23 Engine & Multi-Repo Preferences

The engine is a relational system where **everything is a pointer**.
These laws are hard requirements, not suggestions.

---

## The Law Identity Doctrine (Title Over Number)

> [!WARNING]
> **A law is its Title, never its number.** Every law in this document
> carries exactly one canonical Title — *No Arrow Sugar Law*, *Dest-Last Law*,
> *Teardown Order Law*. The integer that prefixes a section is a **positional
> ordinal**: it keeps the document in a readable order and nothing more. It is
> not the law's identity, it never participates in a citation, and it may
> change at any moment as laws are inserted, promoted, split, or merged.
>
> This is a living document (the *Living Preferences Law*). Numbers drift
> under that life — laws grow, split, and renumber — so a citation pinned to a
> number silently goes stale and starts pointing at the wrong law. A citation
> pinned to a Title is immune to renumbering. **Never quote a number when you
> mean a law.** Everywhere a law is referenced — code annotations
> (`;;INTENTION("per the No Arrow Sugar Law")`), `;;OVERVIEW` headers,
> README/CONTRIBUTING taxonomies, `_docs/*`, commit messages — cite the Title,
> in this exact form:
>
> ```c
> ;;INTENTION("per the Single Class Per File Law + the Living ;;OVERVIEW Blueprint Law")
> ```
>
> **Sub-numbering is abolished.** There are no `11.4` or `35.3` laws.
> Anything that once carried a fractional number is now a whole law with its
> own Title and its own ordinal. The literal `n.n` notation is a defect on
> arrival and must never be re-introduced.

---

## The Law Index (Canonical Titles)

The only authoritative list of law names. A law keeps this Title forever;
its ordinal may move as the document evolves.

| # | Law Title |
| :-- | :--- |
| 1 | No Arrow Sugar Law |
| 2 | Cast Spacing Law |
| 3 | Single Class Per File Law (Java Law) |
| 4 | Function Naming Law |
| 5 | Single-Line If Law |
| 6 | Cohesive Commits Law |
| 7 | No Auto-Pushing Law |
| 8 | Two-Semicolon Annotation Style Law |
| 9 | Dest-Last Law |
| 10 | Two-Layer Access Cap Law |
| 11 | Build & Naming Conventions Law |
| 12 | Window Compositing Layer Order Law |
| 13 | Window Decoupling Law |
| 14 | Single-Transaction Live Coordination Law |
| 15 | No-Transaction-Across-Event-Dispatch Law |
| 16 | Pane-of-Glass Law |
| 17 | Continuous Real-Time Live Resize Law |
| 18 | Native Pixel Law |
| 19 | Panel Gravity Law |
| 20 | Present-On-Demand Law |
| 21 | Commit and Push Discipline Law |
| 22 | Pointer Declaration Spacing Law |
| 23 | Vertical Integration Law (Supervisor Order R1–R5) |
| 24 | One Type Registry Law |
| 25 | Canonical Include Paths Law |
| 26 | Standalone Autonomy Law (Target Seam) |
| 27 | Multi-Repo Atomic Commit Discipline Law |
| 28 | SPIR-V Shader Deployment Law |
| 29 | Identity & Naming Transition Law |
| 30 | Living `;;OVERVIEW` Blueprint Law |
| 31 | Symmetric Getter/Setter Completeness Law |
| 32 | Per-Repo Commit Message Scope Law |
| 33 | Teardown Order Law |
| 34 | Bounded Wait Law |
| 35 | Four System Levels Law |
| 36 | Sub-Part Field Segregation Law |
| 37 | Living Darling Docs Law |
| 38 | AI-First Architecture Manifesto Law |
| 39 | Living Preferences Law |
| 40 | Conflict Triage Law |
| 41 | Asset Sourcing Law |
| 42 | Cold-Strict, Hot-Minimal Validation Law |
| 43 | Data-Oriented Storage Law |
| 44 | Living Feature Readiness Law |
| 45 | Test Segregation Law |
| 46 | Ecosystem Vulkan Safety Nets Law |
| 47 | Dynamic Scalability & Anti-Hardcoding Law |
| 48 | No Section Sign Law |

---

## Separation of Concerns: Rule Taxonomy

To ensure uncompromising architectural consistency across all repositories and contributors (human and AI), the laws are partitioned into three distinct tiers of concern, each defined with its architectural **Definition** and foundational **Why**:

1. **Tier 1: Critical Architectural Invariants & Memory Consistency (Non-Negotiable Core)**
   - *Concern*: Hardware execution safety, zero steady-state allocation, lifetime predictability, thread safety, and crash prevention.
   - *Laws*: the Single Class Per File Law, the Cohesive Commits Law & the Multi-Repo Atomic Commit Discipline Law, the Window Compositing Layer Order Law & the Present-On-Demand Law, the Build & Naming Conventions Law (Apple Silicon native), the Teardown Order Law, the Bounded Wait Law, the Four System Levels Law (L1–L4, distinct from R1–R5 Supervisor Order), the Cold-Strict hot-minimal contract half (never crash/block/allocate/use-after-free), the Test Segregation Law.
   - *The Why*: Violations cause segmentation faults, thread deadlocks, memory leaks, GPU driver crashes, un-bisectable repositories, or codebase pollution.

2. **Tier 2: Semantics, Object Models & Living Contracts**
   - *Concern*: Relational memory layout, object-oriented encapsulation in pure C23, deterministic constructor dispatch, symmetric introspection, and self-documenting code contracts.
   - *Laws*: the Dest-Last Law, the Two-Layer Access Cap Law, the Living `;;OVERVIEW` Blueprint Law (constructor dispatch macros), the Vertical Integration Law (Supervisor Order R1–R5), the One Type Registry Law, the Canonical Include Paths Law & the Standalone Autonomy Law, the Living `;;OVERVIEW` Blueprint Law, the Symmetric Getter/Setter Completeness Law, the Sub-Part Field Segregation Law, the Living Darling Docs Law, the AI-First Architecture Manifesto Law, the Living Preferences Law, the Conflict Triage Law, the Cold-Strict hot-minimal contract half (setter validation policy, truncation flag, seam tests).
   - *The Why*: High-level C code must act as a reliable, predictable class system. Every struct field must have transparent, symmetric access; every class must be fully documented in-place.

3. **Tier 3: Syntactic Aesthetics & Mechanical Determinism**
   - *Concern*: Eliminating ambiguous syntax, visual sugar, and aliasing that obscures pointer operations or impairs machine readability.
   - *Laws*: the No Arrow Sugar Law, the Cast Spacing Law, the Function Naming Law, the Single-Line If Law, the No Auto-Pushing Law, the Two-Semicolon Annotation Style Law, the Pointer Declaration Spacing Law, the Per-Repo Commit Message Scope Law, the No Section Sign Law.
   - *The Why*: The codebase is engineered for AI-human pair systems programming. Machine reasoning thrives on explicit, un-sugared syntax where every dereference is visible and unambiguous.

---

## 1. No Arrow Sugar Law
Never use `->`. Field access is always `(*ptr).field`.

```c
(*this).x = 5;            // yes
this->x = 5;              // no
```

## 2. Cast Spacing Law
A cast has exactly one space to the right of `)`.

```c
uintptr_t addr = (uintptr_t) ptr;      // yes
uintptr_t addr = (uintptr_t)ptr;       // no
```

## 3. Single Class Per File Law (Java Law)
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

## 4. Function Naming Law
- The symbol name in the source is lowercase camelCase (`functionName`).
- It is called as `Class_functionName(params)`.

```c
static uint32_t ticket(uint64_t thread_id) { ... }   // definition

uint32_t t = SpinLock_ticket(0);                    // call site
```

## 5. Single-Line If Law
An `if` with one statement uses no braces — a bare one-liner on the next line.

```c
if(foo)
    (*coo).doo(params);
```

Multi-statement bodies always use braces.

## 6. Cohesive Commits Law
Commits must be strictly cohesive and buildable: **one logical feature or subsystem unit per repository**. Because the ecosystem consists of multiple specialized repositories that are assessed independently, git histories must be clean, traceable, and fully bisectable.
- **Per Repository**: Commits must be executed locally inside the specific repository's git root (`../<repo>`). Never cross repository boundaries in a single commit, and never bundle multiple repos into one untracked commit.
- **Atomic & Bisectable**: Every commit MUST compile cleanly (`-Wall -Wextra -Werror`) and include its build wiring (`CMakeLists.txt`). Broken intermediate states or dead unwired commits are defects that ruin `git bisect`.
- **Cohesive Scope**:
  - Independent classes or isolated fixes commit individually: `feat(cursor): ...`, `fix(label): ...`.
  - Tightly coupled class pairs or cohesive subsystems landing together (e.g. `mesh` + `meshlet`, `brush` + `raster_brush`, `fence` + `semaphore` + `command_buffer`) commit together as a unified functional unit: `feat(mesh): ...`, `feat(paint): ...`, `feat(sync): ...`.
  - Never bundle multiple unrelated subsystems into a single untracked omnibus blob.
- Cross-cutting dependencies commit **upstream-first** per the Multi-Repo Atomic Commit Discipline Law (`vexspoke` -> `graphvex`/`api-haven`/`language`/`darkbase` -> `hotcwap` -> `darling-framework`/`sesh` -> `vexgraph`).
Per-file means per-class file pair: one commit lands the owning `.h` plus its `.c` plus build wiring plus its `;;OVERVIEW` update together and must compile `-Wall -Wextra -Werror`. A literal single-file behavior commit that leaves its pair unbuildable is a defect. Exception (L1-only, the Conflict Triage Law): comment/docs/manifest-only single-file commits with zero struct/API change that compile cleanly are permitted with class scope. Stack large pairs instead of splitting them.

## 7. No Auto-Pushing Law
Never run `git push` on your own. When I explicitly tell you to "push", treat it as a one-off command: execute a single `git push` to sync the repository, and then immediately revert to your default state of never auto-pushing. Regardless of pushing, you must always continue making local, granular commits for every completed feature.

## 8. Two-Semicolon Annotation Style Law
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

## 9. Dest-Last Law
Output parameters come LAST: `(a, b, dest)` / `(left, right, dest)`. Reads
left-to-right like math; the result lands where it belongs, at the end.

```c
Vec4_add(a, b, dest);        // yes
Mat4_multiply(left, right, dest); // yes
Vec4_add(dest, a, b);        // no
```

## 10. Two-Layer Access Cap Law
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

## 11. Build & Naming Conventions Law
- `-Wall -Wextra -Werror`, `-mcpu=native` (host apple-mN; portable across Apple Silicon — baseline `apple-m1`/`generic` if strict M1 compat needed), C23 (gnu23).
- Files are lowercase (`variable.c`, `spin.h`); classes are CapitalCase.
- Structs act as classes: state lives on the struct, behavior lives in
  `Class_functionName(struct Class *self, ...)`.

---

## 12. Window Compositing Layer Order Law (macOS)
The compositing stack from top to bottom is fixed — one window, one blur view,
exactly TWO Metal layers, Vulkan rendered inside:

```
NSWindow Frame       ← NSWindow chrome / traffic lights
└─ NSVisualEffectView ← blur / vibrancy (behind-window blending)
   ├─ CAMetalLayer (bottom)  ← scene/legacy backdrop (scenePanel subtree)
   └─ CAMetalLayer (top)     ← content/UI canvas (contentPanel subtree)
      └─ children rendered in Vulkan:
         COMPOSITED scenes sampled into the board drawable, or
         DIRECT panes = nested sub-layer CAMetalLayers with own swapchains
```

The blur material is everything and nothing: it never presents and it never
re-renders during a drag — it is stable glass behind both layers. Where both
Metal layers are transparent, the blur shows through; the Vulkan children are
what actually paint, and they paint *inside* the two layers, never above them.

- `contentPanel` (top) and `scenePanel` (bottom) are the **two named boards**:
  top owns the UI canvas `CAMetalLayer`, bottom owns the scene/legacy
  `CAMetalLayer` (`PanelCocoa_newBoard`), each with a dedicated `VkPane`
  swapchain. A board paints its whole subtree into its own chain; scene
  children render as COMPOSITED layers sampled into the board pass or as
  DIRECT panes (the Pane-of-Glass Law). Plain UI under the top board needs no
  `IOSurface` — it paints into the board pass.
- `contentPanel` without board backing stays a **pure placeholder**: its own
  background color is ignored and its children fall back to per-child panes.
  A board-backed `contentPanel` paints its whole subtree into its own chain.
- `scenePanel` without board backing (set via `Window_setScenePanel`) renders
  directly into the bottom layer's swapchain background. `NULL` means that
  layer clears to fully transparent, letting blur show through.
- Every `CAMetalLayer` in the stack pins top-left (`contentsGravity
  kCAGravityTopLeft`, `anchorPoint (0,0)`, `geometryFlipped YES`, `(0,0)` =
  top-left) and presents with the WindowServer transaction
  (`presentsWithTransaction YES`), so anchor motion and presents land on the
  same vsync — edge-locked, zero CPU catch-up.
- Calling `Window_setBlur(w, value > 0)` **must** also mark the window transparent
  so Vulkan rebuilds the swapchain with `VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR`.
  This is done automatically inside `Window_setBlur`.
- Presentation is **on demand** (the Present-On-Demand Law): during a live-resize
  drag the presenter wakes on the moving edge and rests on the last composite
  otherwise. Static content is never re-presented.
- Pane register timing: `Darling_initCompositor` runs `Vk_init` first; child
  attach happens inside `Darling_preFrame` →
  `Window_attachPanes`, so `VkPane_register` always finds a live
  device. Metal layers are attached before the composite frame is set, so a
  zero-extent capability report falls back to the registered pixel size —
  never fail registration on a frameless layer.

## 13. Window Decoupling Law (a Window is just a Window)
The graphics loop boots only when a window actually hosts a render surface —
`contentPanel` (top board) or `scenePanel` (bottom board) attached, which
happens when an Application *borrows* the Window and registers it into
graphvex's `GfxLoop`. A bare window with no borrower is a plain AppKit
window: native free resize, zero swapchain, zero warm-up presents, zero
`GfxLoop` registration, zero present thread. Rendering never owns the
window's resize; the swapchain machinery is additive on top of an
already-resizable host window, and the loop lives in graphvex (the Vertical
Integration Law) — never inside the Kernel.

A Window is a **dumb surface + callback bridge**: it carries no presentation
logic of its own — only the `CAMetalLayer` frames, the input adapters, and a
set of exported C functions (the bridge) that graphvex calls to present with
transaction, resize panes, attach boards, and read render generation. The
window never renders, never ticks, and never schedules; it answers the
bridge and gets out of the way.

The No-Transaction-Across-Event-Dispatch Law governs how the bridge's event
pump presents. The window presents nothing on its own — it only publishes
live frames as events; graphvex (R3) consumes them through the
`resizeRenderFn` hook and `GfxLoop_modalTick` seam and presents only when
actually registered (Window is a dumb surface + bridge, never a renderer,
per this law).

## 14. Single-Transaction Live Coordination Law
During a live resize every layer change across BOTH Metal layers lands in ONE
explicit `CATransaction` (frame in logical points + `drawableSize` in native
px per the Native Pixel Law) with implicit layer actions disabled for the drag
— so both edges track the cursor on the same event and neither layer lags the
other. `presentsWithTransaction YES` then makes each drawable swap atomic
with its own motion, and because both layers are sublayers of the same window
subtree committed in that one transaction, the whole composite (blur + bottom
+ top) lands on one vsync.

## 15. No-Transaction-Across-Event-Dispatch Law
No transaction spans `[NSApp sendEvent:]`. A live-resize drag enters AppKit's
modal tracking loop INSIDE `sendEvent` and blocks thread 0 until mouse-up.
Any `CATransaction` open across that call collects every native layer frame
(blur view, content view, pane motion) into ONE commit at mouse-up — the
window freezes mid-drag. The event pump therefore commits+rebegins around
each dispatch: the tracking loop always runs with no outer transaction, AppKit
commits each native drag step on its own runloop turn, and the pump's own
batch commits before/after (idle cadence for `presentsWithTransaction`
drawables).

## 16. Pane-of-Glass Law
A scene child renders either as a COMPOSITED layer (retained offscreen render
targets — the `VkLayer` registry in graphvex — collaged into the canvas by the
composite pass; the default, one `CAMetalLayer` total) or a DIRECT pane (its
own `CAMetalLayer` + dedicated per-pane Vulkan swapchain — `VkPane` registry
in graphvex, `PanelCocoa_newMetal` in darling — every panel is a Vulkan rect).
DIRECT is the managed exception (the Conflict Triage Law) for full-window or
latency-locked scenes that must not pay the composite copy. The flight
machinery — registry, stable slot index, dual flight slots, acquire/render
semaphores, bounded 100ms fences, dirty bit — is shared verbatim between both
modes; only the destination differs: a presentable swapchain image (DIRECT)
vs a compositable color image the canvas samples (COMPOSITED). A layer/pane's
pixel size is FIXED at register/resize time —
`VkPane_resize`/`VkLayer_resize` is a no-op when the requested size is
unchanged, so fixed targets never rebuild.

## 17. Continuous Real-Time Live Resize & Presentation Law (Abolishing "Freeze-Exact")
Freezing swapchain extents, dropping `VK_ERROR_OUT_OF_DATE_KHR` frames, and
early-returning from layout during mouse drags (`Window_isLiveResizing`) is
**strictly abolished**. Deferring work to "settle" is an artificial cop-out
that produces frozen windows, dead animations, and visual tearing. During an
active window drag or live resize, the rendering pipeline operates
continuously:

1. **Dynamic Extent & Swapchain:** `CAMetalLayer.drawableSize` tracks live
   window bounds on every resize event. Swapchain out-of-date events
   immediately rebuild the swapchain cleanly without dropping frames.
2. **Live Layout Recalculation = Real-Time Anchor Feel:** Container layout and
   anchor resolution run on live bounds every frame of the drag — pinned
   elements (e.g., right-anchored, bottom-anchored) recalculate their offsets
   dynamically and re-present, so they stay glued to their edges in real time.
   The resolution granularity is one vsync (60/120Hz), which IS the native
   contract: CoreAnimation itself commits per frame, so per-frame latency is
   indistinguishable from a native view. Nothing is ever stale beyond the
   latency of a single frame. The sub-frame gap between a resize event and
   the next frame is covered by the instantly-moved layer frame (the
   Single-Transaction Live Coordination Law) plus the gravity-pinned last
   bitmap — never black, never torn, never stretched-out-of-anchor.
3. **Unbroken Animation & Presentation:** Animation tickers,
   dirty-propagation, and command buffer presentation
   (`presentsWithTransaction = YES`) continue rendering and presenting at the
   display's native refresh rate (60/120Hz) throughout mouse drags and moves.

## 18. Native Pixel Law
All `CAMetalLayer` `drawableSize`s and Vulkan renders into pane chains must use
**native hardware pixels**, not logical points.

```c
// CORRECT — multiply point size by the monitor scale factor
int pxW = (int)(rect.z * kx + 0.5f);
int pxH = (int)(rect.w * ky + 0.5f);
VkPane_register(layer, pxW, pxH, owner);

// WRONG — logical points only, blurry on Retina
VkPane_register(layer, (int)rect.z, (int)rect.w);
```

The `CALayer` frame is always set in **logical points** (CoreAnimation convention).
`contentsScale` on a Metal pane layer must be set to `backingScaleFactor` (e.g. `2.0` on Retina)
so CoreAnimation maps the native physical pixels of the pane's swapchain to logical points at
exact 1:1 screen resolution, preventing the content from appearing doubled in size.

## 19. Panel Gravity Law
Each Metal pane layer gets its `contentsGravity` and
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

The `CAMetalLayer` hosting a Vulkan swapchain has `geometryFlipped = YES` so
that Vulkan's top-down coordinate space maps correctly onto CoreAnimation's
bottom-up space. Every pane layer in the stack carries it for the
same reason.

## 20. Present-On-Demand Law (composite ≠ render)
A surface presents only on demand. Demand is change: motion, layout, text,
hover, or a scene publishing a new frame. Anything static presents once and
then rests on its last composite — the compositor never re-presents clean
content and never re-invokes a scene's render handler.

### The Why (subsumes the former No Double-Render Law)
The old law forbade stamping one panel into two chains. Present-on-demand
makes that impossible by construction: a scene RENDER (its world into its
retained offscreen target — `VkLayer`, own thread, own FPS) is distinct from
a COMPOSITE (sample published targets + paint the UI tree into the canvas at
vsync). The canvas painter only samples published targets — it cannot
re-render a scene. Double-render is structurally unreachable, not merely
forbidden.

### Present modes (per scene):
- `COMPOSITED` (default): the scene owns retained flight render targets
  (offscreen images + acquire/render semaphores + fences) on its own
  timeline; the canvas samples the latest published frame at the anchor rect,
  in tree z-order interleaved with UI. One canvas total — no per-scene
  surfaces.
- `DIRECT` (managed exception, the Conflict Triage Law): a scene may own its
  own `CAMetalLayer` + swapchain (`VkPane`) and present at its own pace —
  full-window or latency-locked scenes that must not pay the composite copy.

### Composite rules:
- The presenter wakes only on demand: a dirty tree, a published layer frame,
  or a live-resize drag (the moving edge is itself a ticket). An idle tree
  sleeps — zero presents, zero GPU work; power is the free win.
- UI paints the FULL tree on any dirty tick (immediate-on-demand; damage
  rects are a later optimization, never a first move). Static content rests:
  the canvas is neither re-acquired nor presented.
- Scene anchors are plain resolved C rects into the canvas; WindowServer-
  native anchoring (`presentsWithTransaction`, `autoresizingMask`) lives on
  the single canvas layer only.

## 21. Commit and Push Discipline Law
- **Never push unless explicitly asked.** A push request is a one-time button press; do not auto-push subsequent changes.
- **Always commit locally.** Continue implementing granular, per-class local commits regardless of whether a push was requested.
- Do not commit unless the change actually fixes or finishes something — broken
  intermediate states stay dirty on disk.
- Commits are per logical fix/feature per class. Keep classes and features isolated.

## 22. Pointer Declaration Spacing Law
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

Casts are the sole exception: `(T*) var` — no space before `*` inside the cast, then one space after `)` per the Cast Spacing Law:

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

## 23. Vertical Integration Law — Single Order To Follow (R1 > R2 > R3 > R4 > R5)
The stack has ONE order. Lower R = boots earlier, more stable, tears down later. Follow this everywhere.

```
┌────────────────────────────────────────────────────────────────────────┐
│ R1 — HOST (hotcwap)                                                    │
│ Supervisor; Kernel = registries + dispatch. NO Kernel tick, NO pump      │
│ thread — the frame loop + event pump live in R3 graphvex (GfxLoop).     │
│ Hot / Manifest / VkLoader / Window (window_cocoa.m) /                   │
│ process/{Process, Application, Console} — the three lifecycle kinds.    │
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
│ Each = Application x windows[APP_MAX_WINDOWS] in Kernel.                  │
└────────────────────────────────────────────────────────────────────────┘
│ vexgraph — umbrella integrator (projects/, main/vk_test.c, tooling)    │
└────────────────────────────────────────────────────────────────────────┘
```

How to read the arrows (the only two directions in the whole repo):
- `supervises ▼` (runtime): R1 boots R2, loads R3 dylibs, registers R4 UI, R5 apps. Teardown runs reverse per the Teardown Order Law.
- `borrows shape ▲` (compile-time): a file may only `#include` shapes from the allowlist below. Supervisor borrows leaf shapes; leaf never borrows supervisor shapes.
- `registers ◀` (engines): R5 never gets `#include`d by R1. R1 holds `void*`
  + per-kind fn-tables (`ProcessEntry`, `AppRunFn`/`AppTickFn`/`AppHotReloadFn`,
  `ConsoleIo`) + `HotModule`. Same feature, no circular link, standalone
  stays green per the Standalone Autonomy Law. Windowed Applications
  additionally register a frame-handler frame-client into graphvex's
  `GfxLoop` — opaque handle + callback, never an `#include`.

Allowlist (only includes permitted — everything else is a defect):
- R2 `vexspoke`: includes NOTHING from `graphvex`/`hotcwap`/`darling`/`api-haven`/engines. Pure leaf.
- R3 `graphvex`: includes `vexspoke` only. Never `hotcwap`/`darling`/`api-haven`/engines.
- R3 `api-haven`: `vexspoke` only, no graphics; may define pure connector contracts — descriptor registries plus fn-pointer client shapes (e.g. `AiProvider`, `DbProvider`, `McpServer` — `AppDetect`/`CaptureTool`/`ProcessProbe` live in vexspoke R2 and are consumed, never re-implemented) — with zero vendor/database includes; contract class names never collide with owner interfaces (the `Database` interface stays with darkbase). MCP tool/resource surfaces (stdio JSON-RPC engines and their `mcp_server` runners) are connector-shape hosting and live in api-haven; they host handler closures over those registries/probes only — no exec, no writes, no vendor SDKs.
- R3 `language`/`darkbase`: `vexspoke` (+ `graphvex` for GPU-backed ones) only, never engines.
- R1 `hotcwap`: includes `vexspoke` (arena/event/input/time) + `graphvex` (buffer/GPU) only. Never `darling`/`api-haven`/database/language/engine headers.
- R4 `darling-framework`: includes `vexspoke` + `graphvex` + `hotcwap` (`window/window.h`) only. Never `api-haven`/engines.
- R4 `sesh`: `vexspoke` + `api-haven` (WsFanout) only. Never graphics engines.
- R5 engines (`semicolon`, `samplerate`, `darling-editor`, `drawling`, `anti`): borrow shapes from R1/R2/R3/R4 to build; own no OS/window/memory management — borrow arenas, windows, GPU instances from R1. Standalone-capable or Kernel-registered.

Managed exceptions — socket/spawn/decode seams (waivers under the Conflict Triage Law, Tier 1 preserved):
- R2 `vexspoke` owns the `WsClient` + `ProcessSpawn` leaf drivers. `WsClient`
  is a bounded frame slot (fixed rx buffer, state, cancel flag, timeoutNs):
  no threads, no owned sockets — the R1 driver owns the socket and feeds
  bytes in; `WsClient_poll` waits at most 100ms in ~1ms cancel-checked
  slices (the Bounded Wait Law, drop-degrade false). `ProcessSpawn` is a
  bounded child-job table (fixed slots, cancel flag): `posix_spawnp` launch
  (never `system()`), non-blocking `waitpid` reap slices bounded to 100ms,
  `SIGTERM` cancel — no blocking wait, no `UINT64_MAX`, zero threads.
- R3 `graphvex` owns `FrameImporter` (pure RGBA8→`Image` upload via
  `Image_upload`): no `popen`, no `libav*` include/link, no threads; callers
  decode through the `ProcessSpawn` shape and never call it from
  tick/render paths.
- R3 `api-haven` owns `HavenWsFanout` (16-slot fan-out registry over opaque
  `void*` handles + `WsSource` fn-table `{connect, poll, send, close}`,
  `pollStep` budget-driven by R1) + `AiSse` (pure caller-fed incremental
  SSE `data:`/`event:` state machine bound explicitly to one fanout slot
  via the existing `WsSource` table, R1-budgeted `pollStep`, 100ms slices,
  drop-degrade false, zero socket/thread/alloc, truncation flag per the
  Cold-Strict, Hot-Minimal Validation Law) + `DbProvider` catalog-only
  read-only SQLite file row (`DbSqliteFile`: path, caps, bounds, fn-table
  only, zero `sqlite3.h` include/link — execution delegates via opaque handle
  to the database owner / vexspoke File/VFS) + `AssetBroker`
  (`AssetBroker_downloadToCache` bounded chunked-copy
  `(srcChunk, dest, destCap, outTruncated)` dest-last, per-chunk 100ms
  budget + cancel flag, never a whole-file budget, `VexHome_cache`-confined,
  closed before `Memory_freeAll`): zero `pthread_*`, zero socket syscalls,
  no scraping, no exec, no vendor SDK; transports stay in R2, driven by
  R1 callbacks only.

Build/commit order (dependencies first, per the Multi-Repo Atomic Commit
Discipline Law): `vexspoke` -> `graphvex`/`api-haven`/`language`/`darkbase` ->
`hotcwap` -> `darling-framework`/`sesh` -> `semicolon`/`samplerate`/
`darling-editor`/`drawling`/`anti`. Boot order is the reverse crown: R1
first.

1. **R2 `vexspoke` Behavior**:
   - Owns: `Variable`, `BitPool`, `Memory`/`MemoryArena`, `RingBuffer`/`SpinLock`, `Type`/`Class`, math, `http`/`json`, `VexHome`/`File`/`Log`, audio, base Vulkan context, **system capability probes (`AppDetect`, `CaptureTool`, `ProcessProbe`)**.

2. **R3 Drivers — graphvex | api-haven | language | darkbase**:
   - `graphvex`: `spv/` blobs, `Buffer` family, `Font`/`FontBake`, `SdfGpu`, `Texture`, `Raster`, `WgpuBackend`.
   - `api-haven`: API surface, telemetry, webhooks + connector contracts (AI providers, app detection, database catalog/connector shapes) + the MCP tool/resource server (`McpServer`, `mcp_server` stdio runner) hosting them.
   - `language`: `Language` contract (`Lang_tokenize/parse/highlight/...`), each grammar a hot-swappable dylib.
   - `darkbase`: `Database` interface, native vex store in-budget.

3. **R1 `hotcwap` Host**:
   - Owns: `Kernel` (`../hotcwap/kernel/kernel.h`), the **three process
     kinds** in `hotcwap/process/` (`process/process.{c,h}`,
     `process/application.{c,h}`, `process/console.{c,h}`), `Window`,
     `Hot`/`Manifest`/`VkLoader`/`SpvWatch`.
   - **Process taxonomy (classify by the first matching question):**
     1. Presents pixels through a Window/board composite chain
        (`CAMetalLayer` + swapchain)? → **Application** — a pure *manifest*:
        identity (name/author/version/icon), window registry,
        and a hot-module slot. It NEVER owns a tick, a present worker, a
        frame scheduler, or an event router — `Application_start/stop` flip
        the `running` flag, and `Application_run` is the keep-alive parked
        loop (hotcwap's own, graphvex-independent): it BLOCKS until every
        registered window is closed, letting the Window pump its own events
        in 25ms slices and asking closed-state at a 250ms cadence — so an
        empty window lives on its own and `Kernel_run(kernel, app)` returns
        only once the user closed all windows. The Kernel (R1) dispatches the
        Application into graphvex's `GfxLoop` (R3),
        which drives the Thread-0 event pump, frame scheduling, presentation
        (demand-driven, the Present-On-Demand Law + the Continuous Real-Time
        Live Resize Law), and telemetry writes through the Window's C
        callback bridge. window attach/detach lifecycle, present-on-demand.
     2. Talks to a tty/stdio, executes shell scripts, or hosts a REPL/session?
        → **Console** — never a window; borrows R2 `ProcessSpawn`/`File`/
        `VexHome` for execution. A session pump state machine (`ConsoleIo`
        fn-table seam per the Conflict Triage Law), polled in bounded 100ms
        slices with a cancel flag (the Bounded Wait Law) by a supervised
        thread — zero sockets, zero threads owned.
     3. Neither — just a function? → **Process** — one-shot invocable wrapping
        one `int (*ProcessEntry)(void *context)` on the caller's thread.
        Re-runnable, never ticked; one invocation at a time. `Process_run`
        returns admission status separately from the callback's exit status.
        `Process_replace` changes entry/context/hot association together between
        calls, returning busy rather than waiting. Name and context are borrowed;
        freeing requires deregistration and external exclusion of API callers.
        A hot association is not a pin: callers must keep code loaded during
        execution until generation pinning is integrated with the loader.
   - **One type per process.** Classification is the *primary contract*:
     surface > stdio > function. A terminal-in-a-window is composition — an
     Application hosting a child Console via `ProcessSpawn` — never a hybrid.
   - **Kernel is an object that stores and dispatches work — never an
     executor.** `Kernel` owns the arenas, the three registries
     (`processes`/`applications`/`consoles`, all doubling arena slabs per the
     Dynamic Scalability & Anti-Hardcoding Law), and the supervised `Thread`
     registry. There is **no `Kernel_tick`, no present worker field, and no
     Kernel-owned pump thread**: `Kernel_run` is a thin reference forward that
     hands each registered kind to its own run function — `Process_run()` for
     one-shot invokables, `Console_run()` for session pumps, and graphvex
     `GfxLoop` registration for Applications (which then drives the frame
     loop, the Thread-0 event pump, presentation, and `frame(app, dt)`
     handler invocation on live bounds as a demand-driven frame scheduler,
     the Present-On-Demand Law + the Continuous Real-Time Live Resize Law).
     The Kernel never implements the work itself, so hot-reloading a module
     swaps the running code without touching the supervisor.
   - `Application`/`Process`/`Console` are final infrastructure — R5 apps rely
     on them, they never rely on R5. An Application never grows a loop back:
     tick/render/present/poll are graphvex (frame) and Kernel-dispatch
     (start/stop) jobs, steering through the Window's C callback bridge only.

4. **R4 Interfaces — darling-framework | sesh**:
   - `darling-framework`: `Canvas`/`Container`/`Panel`/widgets/compositor/`panel_bridge.c`.
   - `sesh`: Session sync, VPS relay, Cloudflare edge, in-engine bug ingestion.

5. **R5 Interactables**:
   - `semicolon`: Mini IDE (5MB jGRASP/Zed target, shell-out toolchains, xlsx viewer).
   - `samplerate`: Bare-metal DAW (realtime mixer, spatial audio, 3D HRTF).
   - `darling-editor`: Spatial studio (Figma + Miro board, infinite canvas, HTML/SVG export).
   - `drawling`: Drawing studio (GIMP/Krita/FlipaClip target, layers, brushes).
   - `anti`: 3D game engine (5-column editor, bindless, meshlets, physics, darkbase).
   - N x the three process kinds x M windows per `Kernel` — the registries are
     doubling arena slabs (the Dynamic Scalability & Anti-Hardcoding Law, zero
     ceilings): `processes` (invocables), `applications` (windowed),
     `consoles` (sessions).

6. **`vexgraph` (Top-Level Integrator & Application Root)**:
   - The umbrella project that nests the repositories in `../` and builds unified binaries, probes (`main/vk_test.c`), and tooling.

## 24. One Type Registry Law (Project-Scoped Identity, Uniform Per-Project Numbering)
- **Type identity is project-scoped, never class-number-scoped.** A bare class number is meaningless without its project: vexspoke `#3` (`ID_DOUBLE`) and darling `#3` (`ID_CANVAS`) are entirely different types, because their 64-bit ids carry different PROJECT bytes. To identify any id you **must first resolve its project, then switch on the class number within that project's scope**:
  ```c
  switch (Type_arch(id)) {              // project dispatch FIRST
      case ARCH_VEXSPOKE:  ... // then Type_class(id) ranks vexspoke's registry 1..N
      case ARCH_GRAPHVEX:   ... // then Type_class(id) ranks graphvex's registry 1..N
      case ARCH_DARLING:    ... // then Type_class(id) ranks darling's registry 1..N
  }
  ```
  The 64-bit layout `0x F PRPR M W1 W2 PDPD CCCCCCCC` anchors identity on the 8-bit PROJECT byte (`PROJ_VEXSPOKE`, `PROJ_DARLING`, `PROJ_GRAPHVEX`, `PROJ_HOTCWAP`, ...); the 32-bit class field is only an index into that project's registry. `Type_arch` reads the byte out, `Type_class` masks the local number. Every repository owns exactly one class registry in its own `*-type.h` (vexspoke `oop/type.h`, graphvex `graphvex/type.h`, darling `c23/darling-type.h`), numbered **1..N** with gaps allowed — there are **no global hex windows** (id #1 in darling ≠ id #1 in vexspoke).
- **A full id (project bit set) means exact project dispatch. A bare id (project bit zero) means vexspoke's own legacy class space** — `Type_arch(bare)` reports `ARCH_VEXSPOKE` and class numbers resolve through vexspoke's own chain. Cross-project runtime dispatch must therefore pass full ids (`TYPE_*_SINGLETON`) and mask with `Type_class` before comparing against per-class constants; an unmasked comparison against a bare `ID_*` only ever matches vexspoke classes.
- **Single source of truth:** registry macros live only in the repo's `*-type.h`. Widget/class headers define no guarded `ID_X`/`TYPE_X` fallbacks — duplicate definitions are the scattered-registry defect (a pre-state where `richtext_panel.h` and `expandable_list_container.h` both claimed `0x006A`). Views and BEHAVIOR-class registries (`hotcwap` thread ids, etc.) still declare their macros in one centralized winner header per repo.
- **Cross-project parent chains are data, not code:** the child repo grants its chain table once through `Type_registerParents(proj, parents[], count)`, where `parents[i]` is the parent class NUMBER of class # i (`0` = root). Registration is idempotent (re-register replaces), rejects `proj == 0`, non-project-bit, `PROJ_VEXSPOKE`, and `(nullptr, count != 0)`; the slate grows on demand (arena-backed, exponential doubling per the Dynamic Scalability & Anti-Hardcoding Law). An unregistered project byte resolves every class as a root. `Type_isA` keeps the project byte while walking the chain, so `Type_isA(child, bareTarget)` interprets the target as a class number in the child's project.
- Repos with zero registry classes (hotcwap, api-haven today) register nothing; renumbering a repo's `*-type.h` is its own owner-side feature commit (the Cohesive Commits Law / Multi-Repo Atomic Commit Discipline Law), and downstream consumers of changed class numbers are updated upstream-first (`vexspoke` → `graphvex`/`api-haven`/`hotcwap`/`darling-framework`).

## 25. Canonical Include Paths Law (Zero Parent Hops)
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

## 26. Standalone Autonomy Law (Target Seam)
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
        GIT_TAG spoke
    )
    FetchContent_MakeAvailable(vexspoke)
endif()
```

When building inside `vexgraph`, `vexspoke` already exists as an in-tree target. The guard prevents duplicate target definitions and avoids redundant network fetches.

## 27. Multi-Repo Atomic Commit Discipline Law (Per Feature, Per Subsystem, Per Repo)
The Cohesive Commits Law and the No Auto-Pushing Law apply across all repositories:
- **Per-Repository Execution**: When a change touches a class or feature within a repo, commit locally inside that repository's git root (`../<repo>`). Never commit from the umbrella root for sub-repository changes.
- **Atomic Subsystem Isolation**: Keep commits focused to a single class or cohesive subsystem unit (e.g., `feat(cursor): ...`, `feat(text_core): ...`, or `feat(sync): ...`). Classes that operate as a cohesive pipeline land together with their build wiring; unrelated subsystems must never be bundled into a shared omnibus blob.
- **Upstream First**: Cross-cutting changes spanning multiple repositories must commit in strict downward-only dependency order:
  `vexspoke` -> `graphvex`/`api-haven`/`language`/`darkbase` -> `hotcwap` -> `darling-framework`/`sesh` -> R5 apps.

The old order (`vexspoke` -> `graphvex` -> `hotcwap` -> `darling` -> `api-haven` -> `vexgraph`) is retired; `darling` is now a symlink to `darling-framework`.
- **Zero Giant Blobs**: Assessors evaluate each repository's commit history independently. Grouping unrelated subsystems or multiple repos destroys reviewability.
- **Never Auto-Push**: the No Auto-Pushing Law remains absolute. Commit locally, never push unless explicitly requested.
Upstream-first ordering applies to file-pair commits; each repo-local commit is one file pair per above.

## 28. SPIR-V Shader Deployment Law
SPIR-V shaders (`.spv`) are centralized under `../graphvex/shader/` — the single source of truth, laid out by stage:
- `shader/frag/`, `shader/vert/`, `shader/comp/` — GLSL sources (base: `hello_triangle`, `solid_quad`; UI: `texture_quad`, `text_sdf`; compute: `sdf_jfa`, `sdf_combine`).
- `shader/spv/` — compiled blobs (`<name>_<stage>.spv`, bare `<name>.spv` for compute), rebuilt via `shader/build_shaders.sh` (requires `glslangValidator`).
- (Legacy note: sources lived in `hotcwap/vulkan/shaders/` + `darling/vulkan/shaders/`, blobs in per-subsystem `spv/` mirrors — all stale, pending deletion.)
- **`vexspoke` / `darling`**: Own zero blobs; they load via the resolution protocol below. Per-subsystem `spv/` directories, if still present, are stale mirrors pending deletion.

**Runtime Shader Resolution Protocol**:
The runtime loader (`loadSpvAny`) must search in this exact precedence order:
1. `ANTI_SPV_DIR` / `VEX_SPV_DIR` (build-time staging directory `${CMAKE_BINARY_DIR}/spv/`, populated from `../graphvex/shader/spv/`)
2. `<exe_dir>/spv/<name>` (adjacent deployment)
3. `<exe_dir>/../Resources/spv/<name>` (macOS `.app` bundle)
4. CWD-relative paths (`spv/<name>`, `src/vulkan/spv/<name>`)

The top-level `vexgraph` CMake build staging copies all `.spv` blobs from `../graphvex/shader/spv/` into `${CMAKE_BINARY_DIR}/spv/` so all subsystems discover their shaders seamlessly.

## 29. Identity & Naming Transition Law (Anti → Vexspoke / VexHome)
The codebase is actively transitioning from the initial `anti` prototype name to the permanent **`vex`** family identity:
- Engine core: `anti` → `vexspoke` (the central spoke of the graph).
- Engine home directory: `AntiHome` → `VexHome`. Canonical per-platform root (created by `VexHome_ensure()`; `VexHome_cache(subsystem)` builds `<root>/cache/<subsystem>/` with `dictionary.ini` via `VexHome_cacheEnsure`): macOS `~/Library/Application Support/vexgraph`; Linux `$XDG_DATA_HOME/vexgraph` (≈ `~/.local/share/vexgraph`); Windows `%LOCALAPPDATA%\vexgraph`; fallback `$HOME/vex` when the canonical base is unavailable. `$VEX_HOME`, when set and non-empty, overrides all of the above (test seam). Never delete or migrate legacy `~/anti` or `~/vex` automatically.
- Preprocessor definitions: prefer `VEX_*` alongside backwards-compatible `ANTI_*` defines (e.g., `ANTI_SPV_DIR` / `VEX_SPV_DIR`).
- Executable names: `vexspoke_demo` (formerly `anti`) is the headless demo harness inside `vexspoke`, while `vk_test` and full applications live in `vexgraph`.

## 30. Living `;;OVERVIEW` Blueprint Law
`;;OVERVIEW` is the documentation & file layout standard. Every `.c` (and `.m` where applicable) must be self-contained so that a developer can understand the class, its memory layout, and all its capabilities from the first 100–150 lines of the implementation file without having to tab back and forth to the `.h` file.

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
 * LEVEL: L2 — Behavior (Four System Levels Law: L1 metadata / L2 behavior / L3 module / L4 self-mgmt)
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

## 31. Symmetric Getter/Setter Completeness Law (Java-Library Standard)
In `darling` and high-level class abstractions, every state-bearing field on a class struct must provide complete, symmetric getters and setters, exactly like an idiomatic Java or C# library.

A consumer of the library should never have to manually pierce struct internals or violate the Two-Layer Access Cap Law just to inspect simple state:
- If a `Label` has a `char *text`, it must provide `Label_setText(lbl, text)` and `const char *Label_getText(const Label *lbl)`.
- If a `Label` has `fontSize`, it must provide `Label_setFontSize(lbl, size)` and `float Label_getFontSize(const Label *lbl)`.

### Signature Conventions:
1. **Mutators**: `void Class_set<Prop>(Class *self, <Type> val)`
2. **Scalar / Pointer Accessors**: `<Type> Class_get<Prop>(const Class *self)`
3. **Boolean Accessors**: `bool Class_is<Prop>(const Class *self)` or `bool Class_has<Prop>(const Class *self)`
4. **Multi-Value Accessors**: Follow the Dest-Last Law:
   ```c
   void Class_getSize(const Class *self, float *outW, float *outH);
   void Class_getCrop(const Picture *self, float *outX1, float *outY1, float *outX2, float *outY2);
   ```
5. **Null-Safety**: All getters must defensively check if `self` is `nullptr` and return safe defaults (`nullptr`, `0`, `false`, `0.0f`).

## 32. Per-Repo Commit Message Scope Law (No Repo Prefix — Scope to Class/Subsystem)
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
(`window`, `cursor`, `label`, `mesh`, `paint`, `sync`, `compositor`),
following the Cohesive Commits Law: **one logical feature or cohesive
subsystem per repository**. Cross-cutting changes still commit upstream-first
per the Multi-Repo Atomic Commit Discipline Law, each with its own repo-local
message. Scope names the class/subsystem; the unit shipped is its file pair
(`.h+.c`).

## 33. Teardown Order Law (Destroy Top-Down, Free Last)
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
  stops all `Console` sessions (`SIGTERM` children, non-blocking reap per
  the Bounded Wait Law), detaches all `Application`s out of graphvex's
  `GfxLoop`, retires all `Process` hot-mod pins, closes all windows,
  bounded-joins every supervised thread per the Bounded Wait Law, then
  graphvex `Vk_shutdown` (which joins the `GfxLoop` present thread inside its
  own teardown), resets `transientArena`, destroys master `arena` LAST. Never
  free `arena` while any process kind / Window still runs.

## 34. Bounded Wait Law (No Unbounded Waits on Joined Threads)
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

## 35. Four System Levels Law (L1–L4 — File Stability, NOT Runtime Rank R1–R5)
Every file lives on exactly one L level. Stability increases downward; replaceability increases upward. `L` answers "how safe is it to edit this file?" `R` (the Vertical Integration Law) answers "who boots/supervises whom at runtime?" Never mix them: `R1 hotcwap > R2 vexspoke > R3 graphvex/api-haven > R4 interfaces > R5 engines` is supervision; `L1–L4` below is edit-risk. A `Kernel` file is `LEVEL: L4` living at `R1` — write `LEVEL: L4`, never `LEVEL: R1`.

```
L1  FILE METADATA ............ declarative, easily replaced custom stuff —
                                the files other files consume: manifests,
                                descriptors, schemas (hot/manifest.h, JSON).
                                Swappable with zero code changes.
         │ depends on
         ▼
L2  BEHAVIOR ................. how custom structs/classes work: the class
                                API surface — constructors, core functions,
                                setters, getters (the Living `;;OVERVIEW`
                                Blueprint Law registries).
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
- L-levels (file replaceability) are orthogonal to R-levels (the Vertical
  Integration Law runtime supervision R1–R5). A `Kernel` file is
  `LEVEL: L4 — Self-Management` living at `R1 Host`. Never write `LEVEL: R1`
  — levels are L, supervision is R.
- L4 files change rarely and review heavily: a bug at the bottom breaks
  every level above. L1 files change freely: a bad manifest only breaks
  one module load, caught by ABI verification before any swap.
- New files default to the highest level they can live on. Pushing logic
  downward (L2 → L4) needs justification; it makes the foundation bigger.

## 36. Sub-Part Field Segregation Law & the `Class_part_verb` Law
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
- Sub-object pointers are views: borrowed, detach-only, never freed or
  reparented by the owner. Replacing a view (`scrollbar_setBar`,
  `caret_setView`) detaches the old one and attaches the new one; the arena
  owns the memory, the owner owns the relationship.
- The `;;OVERVIEW` STRUCT FIELDS section mirrors the same banners verbatim
  (the Living `;;OVERVIEW` Blueprint Law) so header and source can never
  drift apart.
- Every part field keeps symmetric getters/setters per the Symmetric
  Getter/Setter Completeness Law — the part API is ergonomic precisely so
  nobody reaches for `->`.

### The Container-vs-Panel Law
A Container is a multi-child layer-owner (owns N child
layers, manages attach/detach/surfaces, the Sub-Part Field Segregation Law
parts); a Panel is a single-surface leaf painter (one IOSurface, no child
layer management). Multi-child managers are named *Container and embed Panel
as first member; leaf painters are named *Panel or own widget class.
TabbedContainer is banned — tabs are a mode of SectionContainer.

## 37. Living Darling Docs Law (Zero Drift Between Code and `_docs/darling.md`)
`_docs/darling.md` (1699+ lines: every widget field, compartment, function,
getter/setter, plus compositor / WindowServer / IOSurface / Vulkan) is a
load-bearing artifact, not a snapshot. An out-of-date section is a defect,
same as a stale `;;OVERVIEW` under the Living `;;OVERVIEW` Blueprint Law.

- **Scope — every class root in darling.** Each class struct / file pair under
  `../../projects/darling/` is a root: `Container`, `Panel`, `Canvas`, every widget
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
- **Same-commit law (extends the Cohesive Commits Law and the Living
  `;;OVERVIEW` Blueprint Law).** Code + `;;OVERVIEW` header + the matching
  `_docs/darling.md` section land in the SAME granular per-class commit.
  Never a follow-up "update docs" commit — follow-ups never happen at 2am.
  A commit that changes darling behavior without its docs section is a broken
  intermediate state per the Commit and Push Discipline Law: keep it dirty on
  disk, do not commit.
- **What "updated" means.** Field table row (type, default, compartment,
  role); function entry with exact C signature + side effects (dirty? layout?
  clamp? reparent?); compartment label corrected if the field moved; one `why`
  sentence if the rationale changed; TOC entry if a section is added/renamed.
  If 2am-you cannot reconstruct the behavior from the docs section alone, the
  update was incomplete — say so in the commit message and finish it.
- **Reviewer checklist (every darling commit):**
  1. `;;OVERVIEW` STRUCT FIELDS + FUNCTION REGISTRY mirror the new struct/API?
  2. `_docs/darling.md` section mirrors the new fields/functions/compartments?
  3. Stub-vs-live status corrected (`;;INCOMPLETE` gained or retired)?
  4. Backend sections (sections 41–48) touched if pixels, events, or teardown changed?
Same-commit law is per file pair: code pair plus overview plus the matching
`_docs/darling.md` section land together; splitting them across commits is a
broken intermediate state.

## 38. AI-First Architecture Manifesto Law
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

## 39. Living Preferences Law (Zero Drift for Invariants)
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
   Every implementation across `hotcwap`, `darling`, `vexspoke`, `graphvex`, and `api-haven` must adhere strictly to the laws codified herein. No repository is exempt.
4. **Title-Identity Enforcement**:
   Because this document is living, no law may be cited by its number anywhere — in code, in docs, or in git history — only by its canonical Title per the Law Identity Doctrine. Any stale numeric citation is a defect to fix in the same cycle it is noticed.

## 40. Conflict Triage Law — Managed Exception, Not Veto
### Definition:
When laws conflict, or intent outgrows a law, the answer is never a bare "this violates X." It is "unless you want it, here is how we manage it." The thought prevails; the laws adapt in the same cycle per the Living Preferences Law.

### The Why:
A veto-only system freezes ambition (multi-app Kernel, R1–R5 ecosystem, 30 grammars, game engines). Tier 1 exists to prevent crashes, not to prevent thinking. Every conflict is triaged, given a managed path, and codified so the next agent inherits the decision.

### The Protocol:
1. **Name the tiers:** Tier 1 (crash/leak/deadlock/memory/thread safety) beats Tier 2 (model/contracts) beats Tier 3 (syntax). State which tier each conflicting law lives on.
2. **Assess before blocking:** state applicability first — does the law actually cover this case (link-time vs runtime, single-app vs Kernel multi-app, global vs per-arena)? A misapplied law is not a violation.
3. **Managed exception:** propose the indirection that preserves Tier 1 while granting intent. Canonical moves: opaque handle + callbacks instead of downstream `#include` (keeps the Vertical Integration Law / Standalone Autonomy Law); fixed array + count + getter instead of `**` chains (keeps the Two-Layer Access Cap Law); `MemoryArena_create/freeAll` + bounded-join instead of globals (keeps the Teardown Order Law / Bounded Wait Law); `;;INTENTION("reason")` + `;;DRAFT` markers for Tier 2/3 waivers.
4. **Prefs patch in-cycle:** if intent prevails, draft the exact `preferences.md` wording change now. Tier 1 waivers additionally require an alternate safety proof (no unbounded wait, no use-after-free, no circular link) reviewed heavily. Tier 2/3 waivers require `;;INTENTION` + overview/docs update in the same commit.
5. **Never silent drift:** a managed exception without its prefs + overview + docs update is a defect, same as stale prefs under the Living Preferences Law.

## 41. Asset Sourcing Law — Legal-Sense, First-Class In-App Marketplace Policy
### Definition:
Every external asset source (images, 3D, textures, audio) is a row in api-haven's AssetProvider descriptor registry. Only blessed public APIs and direct-download URLs offered by the source are wired. Sources without a public search API are catalog-only rows with curated static manifests and hand-verified URLs, or excluded. Interface scraping — parsing another service's HTML/JSON to fake a search API, or bypassing auth — is a defect, always.

### The Why:
Legal exposure, broken trust, and brittle integrations come from scraping. A first-class in-app marketplace must be built on explicit contracts, normalized shapes, and license-aware flows — not on reverse-engineered endpoints that vanish or change without notice.

### The Rule:
- **Catalog, not scraping.** Blessed providers: Unsplash, Pexels, Pixabay, Openverse, Wikimedia Commons, Sketchfab, Freesound, Poly Haven, AmbientCG, OpenGameArt, Google Custom Search JSON API. Each is a row in the AssetProvider registry with its public search API. Sources without a public search API (Pinterest, raw Google Images, Kenney, Quaternius, itch.io packs) are catalog-only rows with curated static manifests and hand-verified URLs, or excluded. Interface scraping is a defect.
- **One normalized contract.** Every search result is an AssetRow (provider slug, id, title, author, license family, preview/download URLs, attribution, dimensions/duration, size). The UI never sees provider-specific shapes.
- **License is a field, not a footnote.** Every row carries a license family; attribution is rendered before import; project export fails closed on UNKNOWN license.
- **Downloads land in the cache.** AssetBroker_download streams into VexHome_cache(<subsystem>) with bounded timeouts (the Bounded Wait Law); cache files are shim state tracked and closed before Memory_freeAll (the Teardown Order Law). No exec, no writes outside the cache.
- **The UI seam is fn-pointers.** darling hosts AssetBrowser and never includes api-haven (the Vertical Integration Law); the R5 app binds an AssetSource fn-pointer table (opaque handle + callbacks — the Conflict Triage Law canonical move).
- **MCP surface.** asset_source_lookup / asset_search / asset_download hosted by McpServer; writes cache-confined, timeouts bounded, no exec.
- **Credentials.** API keys via vexspoke Keychain or ASSET_KEY_<SLUG> env rendered by ApiAuth; never stored in the arena, prefs, or repo.

## 42. Cold-Strict, Hot-Minimal Validation Law (Crash-Guard Split)
### Definition:
Validation splits by path temperature. The Tier-1 crash-guard half: no function
ever crashes, blocks unboundedly, allocates, or use-after-frees on null,
out-of-bounds, overflow, cancelled, or timed-out input — it returns `false` or
a Symmetric Getter/Setter Completeness Law safe default instead. The Tier-2
contract half: setters validate at least as strictly as getters, with the
reject-or-clamp policy stated in the `;;OVERVIEW`; getters return safe
defaults per the Symmetric Getter/Setter Completeness Law.

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
   then drop-degrade per the Bounded Wait Law (return `false`, keep old
   content, move on).
2. **Hot paths guard minimally, never log.** Vk present, `Raster`, `SdfGpu`,
   darling layout, `GfxLoop_frame`, `presentFrameLocked`: at most one `nullptr`
   entry guard returning `false`, zero per-element revalidation, zero logging,
   zero allocation. The hot path trusts the cold-validated handle. Deeper
   invariants are proven at compile time (`_Static_assert`) or declared as
   `;;INTENTION("reason")` + `;;DRAFT` per the Conflict Triage Law, never
   re-checked per frame.
   ```c
   if (self == nullptr)
       return false;
   ```
3. **The Truncation-Never-Silent clause.** Copying into a bounded buffer takes
   `(src, dest, destCap, outTruncated)` — dest-last per the Dest-Last Law,
   flag last. On cut: return `false` and set the flag so the caller degrades
   loudly.
   ```c
   bool Class_copyText(const char *src, char *dest, size_t destCap, bool *outTruncated);
   ```
4. **Seam tests at cold boundaries only.** The full
   nullptr / empty / wrong-id / overflow / truncation / cancelled / timeout
   matrix lives at public cold seams in `tests/*.c` `MODULE` harnesses. Hot
   paths carry `nullptr`-guard-only tests — no per-element matrix, no timing
   harness on the frame path.

## 43. Data-Oriented Storage Law & Object-Oriented Ergonomics
### Definition:
Collection patterns (nodes, lists, tables, trees) use data-oriented storage — flat
arrays, index-based relationships, zero pointer chasing — with object-oriented
ergonomic API: class methods, part verbs (the Sub-Part Field Segregation Law),
symmetric getters/setters (the Symmetric Getter/Setter Completeness Law),
dest-last parameters (the Dest-Last Law). This is the default for any
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
2. **API is object-oriented.** Every collection is a class (the Single Class
   Per File Law) with constructors, part verbs (the Sub-Part Field Segregation
   Law), symmetric getters/setters (the Symmetric Getter/Setter Completeness
   Law), and dest-last output parameters (the Dest-Last Law). Consumers never
   pierce internals.
3. **`ExpandableListContainer` + `ExpandableNode` is the canonical instance**
   for hierarchical/nested lists: file trees, task checklists, outlines, mind
   maps, schema viewers. `ExpandableNode` is a behaviourless slot record
   (the Single Class Per File Law SLOT RECORD) owned by
   `ExpandableListContainer`.
4. **Checklists are not a separate class.** A checklist is an
   `ExpandableListContainer` instance with `checklistMode` enabled — never a
   distinct `Checklist` or `TaskList` type. The checkbox lives at child slot 1
   within the row panel, toggled via `setChecked`/`isChecked`.
5. **FileList is composition, not a separate class.** A file tree viewer
   composes `ExpandableListContainer` with R2 `File`/`VFS` data — never a
   standalone `FileList` class. The data is R2; the widget is R4 darling.

## 44. Living Feature Readiness Law (Zero Drift for Status)
### Definition:
Each repository's feature readiness matrix lives in the ecosystem wiki repo (`../../_repositories/.ecosystem/<repo>.md`, rendered as the `[[<repo>]]` wiki pages), one row per feature (container/widget/module/command), each carrying a scope line and a status emoji. The matrix is a **living inventory**, not a snapshot: its status column is the machine-readable handshake the ecosystem uses to know what is real vs stubbed vs absent.

### The Why:
Multi-repo ecosystems rot silently — a header-only dialog or a half-stubbed picker looks "implemented" from the call site until someone depends on it and hits the empty paint. A single, always-current matrix — one row per unit, read by machines and humans alike — makes build-readiness legible at a glance, keeps scope lines honest, and exposes the next structural wedge (the largest contiguous 🟥 block) the moment it appears.

### The Rule:
1. **Same-cycle status law, per file pair.** Any commit that ships, stubs, retires, or re-scopes a feature **must move its `../../_repositories/.ecosystem/<repo>.md` row in the same cycle** — code commit first, wiki row-write immediately after, never a deferred "update checklist" blob (the Cohesive Commits Law / Commit and Push Discipline Law). Code and wiki live in different repos so they ship as separate per-repo commits, but a green-on-disk row that is stale-red on the sheet is still a broken intermediate state.
2. **Status legend (canonical, mirrors the wiki `Home.md` Status Legend):** 💚 98% done, production-ready · 🟩 95% done, implemented & functional · 🟨 85% done, substantially implemented · 🟧 75% done, partial/draft · 🟥 concept/draft, zero working source · ⬜ vital future work, not implemented (⬜ is never "dropped/archived"; it marks an important concept not yet built). A row's scope line is rewritten when the actor or deliverable changes, not just the emoji.
3. **Test proof gates the status.** 🟨 rows carry test names in the scope column (`tests/<name>_test`); a row is never 🟩 before its unit tests pass under `-Wall -Wextra -Werror` (the Cohesive Commits Law). Moving a row up without its proof is inflation; use the Conflict Triage Law (`;;INTENTION`) instead of silently overstating.
4. **Commits are per-checklist-file, per-repo.** The matrix lives as one row-write inside its feature commit; cross-repo rows never bundle (the Multi-Repo Atomic Commit Discipline Law). Code and wiki ship as separate per-repo commits in the same cycle — the code commit carries the behavior, the wiki commit carries the row.
5. **The spearhead is the wedge, not the tail.** The next work item is chosen as the structural keystone that unblocks the largest contiguous block of 🟥 rows (e.g. `OverlayRoot` unblocking the dialog/dropdown family), then the block collapses down the matrix — mirrors the upstream-first law (the Multi-Repo Atomic Commit Discipline Law).

## 45. Test Segregation Law (Zero Source Pollution — No Tests in Source Trees)
### Definition:
Test code and harnesses NEVER reside inside production source directories (`src/`, `darling/`, `render/`, `main/`, `app/`, etc.). All unit tests, integration tests, benchmark harnesses, and test fixtures across the ecosystem live in dedicated test trees partitioned by subsystem under `_tests/<subsystem>/`. Production source trees contain only production classes, headers, and build scripts.

### The Why:
Colocating tests alongside production source files pollutes the clean 1:1 class-to-file architecture (the Single Class Per File Law), confuses directory-based build tools and file watchers, muddles static analysis, degrades search/grep ergonomics, and creates risks of circular dependencies or accidental linkage of test helpers into production shared libraries. A source directory must be purely production code; test suites are clients of the subsystems they test and must sit in segregated test directories.

### The Rule:
1. **Zero test files in production trees.** No file named `*_test.c`, `test_*.c`, `*_test.h`, `test_*.h`, or `*_demo.c` may ever be placed in or committed to a production source directory (`src/`, `darling/`, `render/`, `text/`, `event/`, `app/`, `hot/`, etc.). Violations must be rejected in review and failed in CI.
2. **Unified test hierarchy.** All test sources reside under `_tests/<subsystem>/` (e.g., `_tests/darling/`, `_tests/vexspoke/`, `_tests/graphvex/`, `_tests/hotcwap/`, `_tests/api-haven/`).
3. **Subsystem partitioning.** Tests are grouped strictly by the subsystem they exercise:
   - `_tests/darling/`: UI widgets, containers, text rendering, layout, and event dispatcher tests.
   - `_tests/vexspoke/`: Core primitive, memory, threading, time, io, and relational tests.
   - `_tests/graphvex/`: GPU buffers, rendering passes, texture, and font backend tests.
   - `_tests/hotcwap/`: Hot reload, manifest parser, kernel, and window lifecycle tests.
   - `_tests/api-haven/`: API client, webhooks, MCP server, SSE, and AI provider tests.
4. **Standalone repo test contract.** If a project is checked out standalone without the umbrella `_tests/` root, it must keep its tests segregated in a top-level `tests/` directory at the repo root (e.g. `../<repo>/tests/`), never inside `src/` or component folders. In umbrella builds, `_tests/` is the canonical locus.
5. **No test artifact commits.** Build artifacts, test scratch dumps, and test binaries must be excluded by `.gitignore` (`_tests/` or build output directories).

## 46. Ecosystem Vulkan Safety Nets Law (Determinism + Tree-Shaken Truth)
### Definition:
Vulkan is the one subsystem whose failure surfaces at a *later* boundary than its cause: MoltenVK reports a lost device only at the next API touch, so a `DEVICE LOST` logged at acquire means the damage happened at an earlier call. Every Vulkan-touching file in the ecosystem therefore carries the same two-layer contract: **deterministic handling** (same input sequence ⇒ same seam name, same decision, same log line) and **seam guards** (every driver-facing function validates the health chain before touching the driver). The net spans ALL Vulkan touchpoints — not just the R1 present chain: hotcwap `vulkan.c` (present), `vk_pane.c` (pane presents), graphvex `texture.c`, `sdf_gpu.c`, `vk_scene.c`, `vk_view.c`, `vk_iosurface.c`, darling `compositor.c` (re-record batch). Adding a new file that calls the driver without joining the net is a defect (the repository's class registry alone is too weak — driver calls span repos).

### The Why:
GPU failures are only debuggable if the report site equals the cause site. Chasing acquired-device-lost logs is whack-a-mole; a seam guard converts "the driver died somewhere" into "health broke at seam X" and makes every path to failure reachable, named, and identical — deterministic output producing deterministic debugging. It is not "zero bugs"; it is the guarantee that every failure is reachable, named, and reproducible.

### The Rule:
1. **One canonical guard, zero forks.** `VkGuard_check(seam, device, queue, deviceLost)` lives once in `graphvex/src/vulkan/vk_guard.h` (header-only, no struct, the Single Class Per File Law private-helper doctrine; allowlist-safe: graphvex→own, hotcwap→graphvex, darling→graphvex). It returns `false` when the device is lost or null; a null queue is permitted only on device-resource seams (create/destroy/record) — any seam that submits/fences/presents must pass its queue so the whole queue chain is covered. Callers degrade exactly like any transient failure: return `false`, keep dirty state, retry next tick (the Bounded Wait Law + the Cold-Strict, Hot-Minimal Validation Law). Never a crash, never UB, never a wedged wait.
2. **Tree-shaken when released.** Under `NDEBUG` the guard is a macro no-op: zero calls, zero branches, unevaluated arguments. Release binaries are the Cold-Strict, Hot-Minimal Validation Law hot-minimal skeleton of the debug build — the debug net never counts against hot-minimal branch budgets (the Conflict Triage Law triage: debug exhaustive / release minimal is the managed exception, codified here). Every file's `;;OVERVIEW` lists which of its functions carry the net.
3. **Deterministic driver-state handling.** Every wait/acquire/submit/present follows one fixed decision table, written once: success advances; timeout-with-signal recovers; timeout-unsignaled drops with dirty state intact and retries next tick; `VK_ERROR_DEVICE_LOST` latches once at the true site (`presentDeviceLost(where)` in hotcwap — the latch is single-owned; downstream repos must not re-implement it, they may query `Vk_isDeviceLost()` wherever the allowlist permits) and short-circuits every later pass.
4. **The report site is never the cause.** First action on any `VK_ERROR_DEVICE_LOST`: run with `MVK_CONFIG_LOG_LEVEL` enabled and read MoltenVK's underlying Metal error (`MTLCommandBuffer` error code + message) BEFORE touching code — `MTLCommandBufferErrorInternal`/`PageFault`/`Timeout` distinguishes a usage defect from a GPU power/restart event. Paste both lines together; never "fix the acquire" until MoltenVK says the acquire is the cause.
5. **No loopholes in the health chain.** Handles are nulled in the same teardown pass (the Teardown Order Law) so a stale guard catches a real lifecycle defect instead of passing on a zombie pointer. Cold resource-creation paths (graphvex images/textures/framebuffers/views) keep the Cold-Strict, Hot-Minimal Validation Law result checks; hot per-frame seams (present, pane present, compositor batch, uploads, SDF dispatch, IOSurface export) carry `VkGuard_check` at entry.

## 47. Dynamic Scalability & Anti-Hardcoding Law (No Artificial Limits)

### Definition:
No algorithm, container, layout engine, or rendering pass may ever hardcode fixed task counts, capacity ceilings, or artificial element limits (e.g. `for (int i = 0; i < 4; i++)`, fixed array sizes for dynamic entities, or assumptions like "there are only 2 panels"). Systems must be engineered to handle whatever volume, resolution, or throughput is thrown at them — scaling seamlessly from 0 to $N$.

### The Why:
Hardcoded iteration limits and static capacity assumptions turn code into throwaway prototypes. When an engine assumes a fixed count or bakes dimensions (like hardcoded `640x400` or fixed 4 corners), any real-world workload breaks it. True systems architecture is scale-invariant: the same code that handles 1 child must handle 10,000 children with zero structural rewrites.

### The Rule:
1. **No Hardcoded Loops for Dynamic Work:**
   Writing loops bounded by magic constants (`i < 4`, `i < 2`) to perform structural tasks is a defect. Iteration must be driven by dynamic child counts, queryable collections, or data-driven descriptor streams.
2. **No Capacity Ceilings:**
   Containers, layer registries, viewports, and pass managers must not impose arbitrary hard limits that reject or ignore elements beyond a static constant. Where fixed memory pools are required for zero steady-state allocation (the Data-Oriented Storage Law), storage must grow exponentially or re-index dynamically.
3. **No "Cheat" Modes or Motion Gates:**
   Gating or crippling functionality behind flags like `isLiveResizing` or "only at rest" to avoid implementing the general real-time case is forbidden. If a system can do it at idle, it must be engineered to do it under continuous motion and resize stress.
4. **Generalized Geometry & Anchor Math:**
   All positioning, anchoring, and layout math must be computed dynamically from parent extents $(W, H)$ via relative ratios or anchor matrices, never baked to static pixel constants.

---

## 48. No Section Sign Law

### Definition:
The section sign (§, U+00A7) — the "double-S" — is forbidden everywhere: source comments, `;;OVERVIEW` blocks, docs, commit messages, wiki rows, and this document. Section references are always written as plain ASCII words: "see section 32", "the KeyMap section", "sections 41–48" — never "§32", "§KeyMap", or "§41–§48".

### The Why:
The glyph renders as an ugly double-S that reads as a typo in monospace, breaks `grep` for section references, and mangles in fonts, terminal pipelines, and localized tooling. The word "section" costs nothing and survives every tool, font, and copy-paste intact. A codebase that already bans arrow sugar for machine-readability has no business smuggling invisible punctuation into comments.

### The Rule:
1. **Never write §.** New code, new docs, new commits: the character is a defect on arrival, same as `->` under the No Arrow Sugar Law. Write "section" (or drop the marker) instead.
2. **Migration on touch.** Existing occurrences migrate when their file is next modified: any commit that touches a file containing § must scrub those occurrences in the same commit. A § surviving a touch is a defect (the Living Preferences Law zero-drift rule applies to this migration too).
3. **Canonical artifacts migrate with this law.** The occurrences present in `preferences.md` and the canonical docs at the time this law lands are scrubbed in this same commit; the umbrella-local legacy markers (`// §N` test-section comments, `_docs/code.txt` notes) migrate file-by-file as each is next touched.