# vexgraph's vexspoke — C23 Engine & Multi-Repo Preferences

The engine is a relational system where **everything is a pointer**.
These laws are hard requirements, not suggestions. This document is the
**Universal Supreme Constitution** governing all repositories across the vexgraph ecosystem.
Repo-specific laws and subsystem slices are codified in repo-local `<repo>-preferences.md` files
per the *Per-Repo Preferences Extension Law*.

---

## The Law Identity Doctrine (Title Over Number)

> [!WARNING]
> **A law is its Title, never its number.** Every law in this document
> carries exactly one canonical Title — *Semantic Consistency Law*, *Vertical Integration Law*.
> The integer that prefixes a section is a **positional ordinal**: it keeps
> the document in a readable order and nothing more. It is
> not the law's identity, it never participates in a citation, and it may
> change at any moment as laws are inserted, promoted, split, or merged.
>
> This is a living document (the *Living Documentation Law*). Numbers drift
> under that life — laws grow, split, and renumber — so a citation pinned to a
> number silently goes stale and starts pointing at the wrong law. A citation
> pinned to an active Title is immune to renumbering. A deliberate merge
> records former Titles as historical aliases; new citations use the replacement
> Title and its named clause. **Never quote a number when you
> mean a law.** Everywhere a law is referenced — code annotations
> (`;;INTENTION("per the Semantic Consistency Law (Reference form)")`), `;;OVERVIEW` headers,
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

The only authoritative list of universal law names. Each active law has one canonical Title; ordinals may move. A deliberate merge retires old titles with a historical alias map in the replacement law; new citations use only the current Index title.

| # | Law Title |
| :-- | :--- |
| 1 | Semantic Consistency Law |
| 2 | Single Class Per File Law (Java Law) |
| 3 | Two-Semicolon Annotation Style Law |
| 4 | Build & Naming Conventions Law |
| 5 | Git Workflow Law |
| 6 | Vertical Integration Law — Single Order To Follow (R1 > R2 > R3 > R4 > R5) |
| 7 | One Type Registry Law (Project-Scoped Identity, Uniform Per-Project Numbering) |
| 8 | Canonical Include Paths Law (Zero Parent Hops) |
| 9 | Standalone Autonomy Law (Target Seam) |
| 10 | Living Documentation Law |
| 11 | Bounded Wait Law (No Unbounded Waits on Joined Threads) |
| 12 | AI-First Architecture Manifesto Law |
| 13 | Conflict Triage Law — Managed Exception, Not Veto |
| 14 | Cold-Strict, Hot-Minimal Validation Law (Crash-Guard Split) |
| 15 | Data-Oriented Storage Law & Object-Oriented Ergonomics |
| 16 | Test Segregation Law (Zero Source Pollution — No Tests in Source Trees) |
| 17 | No Section Sign Law |
| 18 | Per-Repo Preferences Extension Law |
| 19 | toString Law (Every Object Has a String) |
| 20 | Authorial Intent Law |
| 21 | No Hardcoding Law |
| 22 | WHAT Law |
| 23 | Cold-Only Reflection Law |
| 24 | Install Ledger Law (Machine-Scoped Install Memory) |
| 25 | Platform Support Floor Law (Apple Silicon macOS 14+, Windows 10+) |
| 26 | Capability Gating Law (Runtime Features Above the Floor) |
| 27 | THROW Law (Loud Cold Rejection) |
| 28 | Native Pixel Law |

---

## Separation of Concerns: Rule Taxonomy

The Semantic Consistency Law spans semantic access/construction/call ordering and syntactic spelling/naming; the Index lists it once, and both tiers apply to its clauses.

To ensure uncompromising architectural consistency across all repositories and contributors (human and AI), the universal laws are partitioned into three distinct tiers of concern, each defined with its architectural **Definition** and foundational **Why**:

1. **Tier 1: Critical Architectural Invariants & Memory Consistency (Non-Negotiable Core)**
   - *Concern*: Hardware execution safety, zero steady-state allocation, lifetime predictability, thread safety, and crash prevention.
   - *Laws*: the Single Class Per File Law, the Build & Naming Conventions Law (Apple Silicon native), the Platform Support Floor Law (Apple Silicon macOS 14+, Windows 10+), the Capability Gating Law (Runtime Features Above the Floor), the THROW Law (Loud Cold Rejection), the Vertical Integration Law (teardown), the Bounded Wait Law, the Cold-Only Reflection Law, the Cold-Strict hot-minimal contract half (never crash/block/allocate/use-after-free), the Test Segregation Law, the Native Pixel Law.
   - *The Why*: Violations cause segmentation faults, thread deadlocks, memory leaks, GPU driver crashes, un-bisectable repositories, or codebase pollution.

2. **Tier 2: Semantics, Object Models & Living Contracts**
   - *Concern*: Relational memory layout, object-oriented encapsulation in pure C23, deterministic constructor dispatch, symmetric introspection, and self-documenting code contracts.
   - *Laws*: the Semantic Consistency Law, the Single Class Per File Law (Java Law) (symmetric accessors), the Living Documentation Law (blueprints, preferences, readiness), the Vertical Integration Law (Supervisor Order R1–R5), the One Type Registry Law, the Canonical Include Paths Law & the Standalone Autonomy Law, the toString Law, the Authorial Intent Law, the No Hardcoding Law, the WHAT Law, the AI-First Architecture Manifesto Law, the Conflict Triage Law, the Cold-Strict hot-minimal contract half (setter validation policy, truncation flag, seam tests), the Data-Oriented Storage Law, the Per-Repo Preferences Extension Law.
   - *The Why*: High-level C code must act as a reliable, predictable class system. Every struct field must have transparent, symmetric access; every class must be fully documented in-place.

3. **Tier 3: Syntactic Aesthetics & Mechanical Determinism**
   - *Concern*: Eliminating ambiguous syntax, visual sugar, and aliasing that obscures pointer operations or impairs machine readability.
   - *Laws*: the Semantic Consistency Law (reference form, control flow, cast and pointer spelling, function and field naming), the Two-Semicolon Annotation Style Law, the No Section Sign Law.
   - *The Why*: The codebase is engineered for AI-human pair systems programming. Machine reasoning thrives on explicit, un-sugared syntax where every dereference is visible and unambiguous.

---


## 1. Semantic Consistency Law

One canonical dialect makes references, access depth, control flow, spelling, naming, calls, and construction predictable across repositories. These are binding clauses of **one** law, not separately indexed laws. A clause name identifies the specific check; cite `Semantic Consistency Law (Access depth)`, for example. This consolidation changes documentation identity, not the existing C ABI or the requirements below.

### Reference form

Never use `->`. Field access is always `(*ptr).field`.

```c
(*this).x = 5;            // yes
this->x = 5;              // no
```

### Access depth

A member/index chain touches at most TWO layers deep:

```c
(*layer1).layer2             // yes — the deepest a chain goes
(*p).items[i]                // yes — indexing rides on its base hop
(*(*ptr).field).field2       // no — three layers
obj.field.field2.field3      // no — three layers
```

Anything deeper must hoist an intermediate into a local first
(`Field *f = &(*layout).items[i];` then `(*f).offset`).

#### The Rationale (Java Object References & Eliminating Pointer Chasing):
In Java, an object reference (`Car car = new Car();`) is never an inline struct; it is purely a pointer under the hood. Instead of hiding behind syntactical illusions or garbage collection, `vexspoke` **embraces the pointer directly**.
When high-level languages allow arbitrary dot-chaining (`car.engine.turbo.valve.pressure`), software falls into the trap of **pointer chasing**—drifting from address to pointer to pointer across disparate memory pages, thrashing CPU cache lines and obscuring memory latency.
Physical hardware memory access is fundamentally simple: **one level + offset**. That is precisely what `(*ptr).field` is: `base_address + field_offset`.
By capping access to at most two layers, pointer hops remain explicit, measurable, and bounded. Accessing a deeper child requires hoisting it into a local variable (`Engine *e = (*car).engine;`), making every memory hop deliberate, visible in machine registers, and impossible to overlook. That's how simple it is.

### Control flow

An `if`, `while`, or `for` with a single-statement body uses no braces — place that statement on the next line. This rule concerns the body, not the number of expressions or arguments in the condition.

```c
if (foo)
    (*coo).doo(params);
```

The same layout applies to `while` and `for`:

```c
while (ready)
    tick();
for (size_t i = 0; i < count; ++i)
    visit(i);
```

Multi-statement bodies always use braces.

### Cast and pointer spelling

A cast has exactly one space to the right of `)`.

```c
uintptr_t addr = (uintptr_t) ptr;      // yes
uintptr_t addr = (uintptr_t)ptr;       // no
```

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

Casts are the sole exception: `(T*) var` — no space before `*` inside the cast, then one space after `)` per the cast-spacing clause:

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

### Function and field naming

- The symbol name in the source is lowercase camelCase (`functionName`).
- It is called as `ClassName_functionName(params)`.

```c
static uint32_t ticket(uint64_t thread_id) { ... }   // definition
uint32_t t = SpinLock_ticket(0);                      // class operation
```

For a new public operation on a named field or sub-part, use `ClassName_fieldName_verbVerbName(...)`; `fieldName` is lower camelCase and `verbVerbName` begins with the operation verb. For an operation on the class as a whole, use `ClassName_functionName(...)`. A field-specific name does not change ownership or permit direct field piercing; repo-local part contracts may impose stricter APIs. Existing ABI symbols are not renamed by this documentation change.

### Argument order

Output parameters come LAST: `(a, b, dest)` / `(left, right, dest)`. Reads
left-to-right like math; the result lands where it belongs, at the end.

```c
Vec4_add(a, b, dest);        // yes
Mat4_multiply(left, right, dest); // yes
Vec4_add(dest, a, b);        // no
```

### Construction and arity

#### Definition:
Every class across the ecosystem ships a **uniform, arity-based convenience surface**, so an object is instantiated, extended, zeroed, and named with canonical constants — never with ad-hoc literals or per-class bespoke spellings. It binds **every framework**, because every part speaks arity.

1. **Macro arity constructor — the one public spelling.** Every public class provides `Class_0()`, `Class_1(a)`, `Class_2(a, b)`, … (the Vec4 chooser idiom) plus a `Class(...)` dispatch macro that selects by argument count. The numbered forms are the macro's **internal expansion targets, not a public surface**. The only construction spelling at any call site — production, test, or consumer — is `Class(...)`: `Frame *f = Frame();`, `Window *w = Window("main", 800, 600);`. A direct `Class_0()` / `Class_1(a)` / … call outside the owning file pair is a defect, and a bespoke `make_*` / `new_*` / `create_*` / `*_new` / `*_create` / `*_make` factory is forbidden everywhere.
2. **`Class_add(...)`.** Every additive or collection class provides `Class_add(...)` — the class's natural additive/append verb (list add, vector add, set union, string append, …).
3. **`Class_zero()`.** Every class provides `Class_zero()` (or a `ZERO` constant) — the additive identity / empty element.
4. **Canonical named constants.** Numeric magnitudes and extremes are named once, canonically: `ZERO`, `ONE`, `ONE_MILLION`, `ONE_BILLION`, `ONE_TRILLION`, `FLOAT_POS_INF`, `FLOAT_NEG_INF`, `DOUBLE_POS_INF`, `DOUBLE_NEG_INF`, `INT_MAX_VALUE`, `INT_MIN_VALUE`, `TYPE_MAX_VALUE`, `TYPE_MIN_VALUE`, and many more — so a call site writes `-ONE` and reads -1, never a bare `-1` literal.

#### The Why:
**Tesler's Law (the Law of Conservation of Complexity):** *"Every application has an inherent amount of complexity that cannot be removed or hidden. Instead, it must be dealt with, either in product development or in user interaction."* — Larry Tesler.

The complexity of *"how do I instantiate this / extend it / zero it / name its extremes"* cannot be deleted — it has to live somewhere. This law pays that complexity **once**, in the library (product development), and hands every developer a uniform, arity-based, constructive surface (the user-interaction side) — so a call site never invents a spelling, and the convenience sits with the developer because the library already absorbed the cost. It is a **universal** law: every framework ships the same arity surface, so the whole ecosystem reads as one language. The numbered `Class_N(...)` functions are that macro's plumbing; promoting them to a call-site spelling would fork the one constructor back into per-class bespoke spellings and defeat the whole surface.

#### The Rule:
1. **Every public class ships `Class_0()`, `Class_1()`, … plus the `Class(...)` arity chooser, and the chooser is the constructor.** The `Class(...)` macro is the public surface; the numbered `Class_N(...)` forms exist only so the chooser resolves and are internal to the owning file pair. No class is constructible only through a bespoke factory name.
2. **Call sites use only the macro form.** No `Class_0()`, `Class_1(a)`, … and no `make_*` / `new_*` / `create_*` / `*_new` / `*_create` / `*_make` spelling may appear outside the class's own `.h`/`.c` pair — including in `tests/<subsystem>/` harnesses, which are ordinary call sites and construct with `Class(...)`.
3. **The numbered forms may name each other only inside the owning implementation.** The `.c` definitions of `Class_0`, `Class_1`, … may delegate to one another; nothing else may call them.
4. **Every additive/collection class ships `Class_add(...)`.**
5. **Every class ships `Class_zero()` (or `ZERO`).**
6. **Canonical numeric constants live in one header.** No bare `-1`, `1000000`, or `INFINITY` literals at a call site — the name is the contract.
7. **Uniform across repos.** Same names, same shapes everywhere — the convenience surface is identical in every framework (the Per-Repo Preferences Extension Law restates it, never forks it).
8. **Managed exception — generic-family tokens.** A class whose bare name is already a generic-family macro cannot also carry a variadic chooser under the same token: `reactive/generic.h` defines `#define Reactive(T) VEX_CAT(Reactive, T)`, so `Reactive(int)` / `Reactive(Vec4)` are the typed constructors and `Reactive(...)` cannot coexist (one macro name). Such a family keeps its numbered bare-engine constructors (`Reactive_1`, `Reactive_2`) as the sanctioned spelling and is exempt from rule 2 by explicit `;;INTENTION` — never by silence (the Conflict Triage Law). `Reactive` is the only such token in the ecosystem; a new one requires an amendment here.

### Legacy title map

Older documents and code comments may cite the former titles below. They remain historical aliases for the listed clauses, **not** additional laws in the Index. New citations use the canonical title and clause.

| Former title | Clause |
| :--- | :--- |
| No Arrow Sugar Law | Reference form |
| Two-Layer Access Cap Law | Access depth |
| Single-Line If Law | Control flow |
| Cast Spacing Law | Cast and pointer spelling |
| Pointer Declaration Spacing Law | Cast and pointer spelling |
| Arity and Constructive Convenience Law | Construction and arity |
| Function Naming Law | Function and field naming |
| Dest-Last Law | Argument order |

---

## 2. Single Class Per File Law (Java Law)

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

### Symmetric accessors

In `darling` and high-level class abstractions, every state-bearing field on a class struct must provide complete, symmetric getters and setters, exactly like an idiomatic Java or C# library.

A consumer of the library should never have to manually pierce struct internals or violate the Semantic Consistency Law (Access depth) just to inspect simple state:
- If a `Label` has a `char *text`, it must provide `Label_setText(lbl, text)` and `const char *Label_getText(const Label *lbl)`.
- If a `Label` has `fontSize`, it must provide `Label_setFontSize(lbl, size)` and `float Label_getFontSize(const Label *lbl)`.

#### Signature Conventions:
1. **Mutators**: `void Class_set<Prop>(Class *self, <Type> val)`
2. **Scalar / Pointer Accessors**: `<Type> Class_get<Prop>(const Class *self)`
3. **Boolean Accessors**: `bool Class_is<Prop>(const Class *self)` or `bool Class_has<Prop>(const Class *self)`
4. **Multi-Value Accessors**: Follow the Semantic Consistency Law (Argument order):
   ```c
   void Class_getSize(const Class *self, float *outW, float *outH);
   void Class_getCrop(const Picture *self, float *outX1, float *outY1, float *outX2, float *outY2);
   ```
5. **Null-Safety**: All getters must defensively check if `self` is `nullptr` and return safe defaults (`nullptr`, `0`, `false`, `0.0f`).

### Legacy title map

Older references to *Symmetric Getter/Setter Completeness Law* refer to the symmetric-accessors clause of the Single Class Per File Law (Java Law). Use the current Index title for new citations.

---

## 3. Two-Semicolon Annotation Style Law

Annotations (`src/annotation/*.h`) are written with two semicolons on the left
side only, so they read as explicit markers:

```c
;;OVERVIEW
;;DEFINITION
;;GETTER
;;SETTER
;;DRAFT
;;INCOMPLETE
;;PLATFORM_EXCLUSIVE("Windows")
;;INTENTION("reason")
;;INHERITS("Base")
;;REACTIVE("objectName")
;;WHAT("uint64_t")
;;CHECKER
;;HOTCODE
```

Two semicolons on the left, nothing on the right — even when the annotation
carries params. The semicolons are plain null declarations; the marker macro
inside expands to a `_Static_assert` that validates the annotation text.

`;;CHECKER` is an optional source marker for an existing cold validator, not
a mandatory runtime checker. It expands to a compile-time assertion with no
runtime work; tests prove the actual validation and safe error result. Never
compile out validation that protects an external or unsafe boundary.

`;;HOTCODE` is an optional source marker for a measured hot path, not proof of
branch count or allocation behavior. It has no runtime work; a registered test
must establish the claimed hot-path properties.

---

## 4. Build & Naming Conventions Law

- `-Wall -Wextra -Werror` and C23 (gnu23). Shipped Apple Silicon targets use the minimum compatible CPU baseline (`-mcpu=apple-m1`); `-mcpu=native` is only for an explicitly local, non-shipped build (the Platform Support Floor Law). Newer features are gated by the Capability Gating Law, not baked into the baseline.
- Files are lowercase (`variable.c`, `spin.h`); classes are CapitalCase.
- Structs act as classes: state lives on the struct, behavior lives in
  `Class_functionName(struct Class *self, ...)`.

---

## 5. Git Workflow Law

The Git workflow is document → verify → commit locally in the owning repository → push only on an explicit, one-off instruction. No step implies permission for the next except as stated below.

1. **Document and verify.** Record what changed, why, and the verification actually performed in the code, tests, relevant docs, or commit message. Build applicable targets with `-Wall -Wextra -Werror` and run relevant tests before committing. Keep broken intermediate states uncommitted; do not claim tests that did not run. The Living Documentation Law owns same-cycle updates.
2. **Commit a cohesive unit in its own repository.** A commit is one independently buildable class pair or cohesive subsystem, including its header, implementation, build wiring, tests, and updated overview when applicable. An isolated docs/manifest-only change may stand alone when it changes no API or behavior. Related class pairs may land together; unrelated changes must not. Never stage unrelated pre-existing edits or treat the local-only umbrella as the owning sub-repository.
3. **Commit dependent work upstream-first.** `vexspoke` → `graphvex`/`api-haven`/`language`/`darkbase` → `hotcwap` → `darling-framework`/`sesh` → R5 applications. Each changed repository gets its own local commit.
4. **Scope the message locally.** A commit message names its class, subsystem, or seam, not its repository. Use `feat(cursor): ...`, `fix(label): ...`, or `test(mesh): ...`, not `feat(hotcwap): ...` within Hotcwap. The message describes the actual cohesive unit, not an unrelated bundle.
5. **Never auto-push.** Only an explicit instruction to push authorizes a single push of the requested repository or repositories; afterward return to the no-push default. Documentation, passing tests, and local commits never grant push permission.

### Legacy title map

*Cohesive Commits Law*, *No Auto-Pushing Law*, *Commit and Push Discipline Law*, *Multi-Repo Atomic Commit Discipline Law*, and *Per-Repo Commit Message Scope Law* are former names for requirements now owned by the Git Workflow Law. New citations use the current Index title.

---

## 6. Vertical Integration Law — Single Order To Follow (R1 > R2 > R3 > R4 > R5)

The stack has ONE lifecycle order: boot from the bottom up (R1 → R2 → R3 → R4 → R5), then tear down from the top down (R5 → R4 → R3 → R2 → R1). Lower R boots earlier, remains alive while borrowers exist, and tears down later. Follow this everywhere. The R ranks describe supervision and lifetime, not the direction of header includes.

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
- `supervises ▼` (runtime): R1 boots R2, loads R3 dylibs, registers R4 UI, R5 apps. Teardown runs top-down in the reverse order (R5 → R1) under this same law.
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
   - `Reactive` (the per-variable event emitter) carries an **atomic** payload + dirty flag, so `Reactive_set` is safe from ANY thread; notification is **owner-affine** — `Reactive_set` never fires observers, the owner (Thread 0 pump/paint) calls `Reactive_drain`, which coalesces the pending writes into one `onSet`/`onChanged` batch on the owner's thread. This keeps the atomicity in the value (no lock, no unbounded wait — the Bounded Wait Law) while keeping every observer on the owner's thread (the Present-On-Demand Law applied to data); the observer lists themselves stay owner-affine.

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
     No Hardcoding Law), and the supervised `Thread`
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
     doubling arena slabs (the No Hardcoding Law, zero
     ceilings): `processes` (invocables), `applications` (windowed),
     `consoles` (sessions).

6. **`vexgraph` (Top-Level Integrator & Application Root)**:
   - The umbrella project that nests the repositories in `../` and builds unified binaries, probes (`../../trash/main/vk_test.c`), and tooling.

### Teardown (top-down; arena last)

Shutdown runs the stack in reverse, and `Memory_freeAll` is always the final
step — never earlier. Shims allocate outside the slabs (`calloc`,
`IOSurfaceCreate`, `strdup`), so freeing the arena first orphans every
surface, layer, and native handle with no record of the leak.

```
R5 applications detach and stop using their borrowed resources
  → R4 frames/compositors detach views, layers, and panel surfaces
    → R3 drivers retire GPU/device resources after dependent work stops
      → R2 behavior workers stop and join before their owned state dies
        → R1 host closes windows, releases remaining native handles
          → Memory_freeAll()      // arena reset LAST, after all teardowns
```

- Detach before free (Lesson 9): registry → parent → free. `PanelCocoa_free`
  must run before its `Panel` is freed; a `Window` detaches adapters before
  `close`.
- A skipped step is a leak, not a shortcut. If a probe exits without
  `Window_destroy`, the `NSWindow` outlives the process as a ghost.
- Multi-app Kernel order (`R1` host, N apps x M windows): `Kernel_destroy`
  first stops R5 clients, then detaches R4 frames and all `Application`s
  from graphvex's `GfxLoop`. It retires R3 graphics resources (including
  `Vk_shutdown` and its bounded present-thread join), stops `Console` sessions
  (`SIGTERM` children, non-blocking reap per the Bounded Wait Law), retires
  `Process` hot-mod pins, bounded-joins supervised workers before releasing
  their owned state, closes R1 windows after adapters detach, resets
  `transientArena`, and destroys master `arena` LAST. Never
  free `arena` while any process kind / Window still runs.

### Legacy title map

Older citations of *Teardown Order Law* refer to the teardown clause of the Vertical Integration Law. New citations use the current Index title.

---

## 7. One Type Registry Law (Project-Scoped Identity, Uniform Per-Project Numbering)

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
- **Cross-project parent chains are data, not code:** the child repo grants its chain table once through `Type_registerParents(proj, parents[], count)`, where `parents[i]` is the parent class NUMBER of class # i (`0` = root). Registration is idempotent (re-register replaces), rejects `proj == 0`, non-project-bit, `PROJ_VEXSPOKE`, and `(nullptr, count != 0)`; the slate grows on demand (arena-backed, exponential doubling per the No Hardcoding Law). An unregistered project byte resolves every class as a root. `Type_isA` keeps the project byte while walking the chain, so `Type_isA(child, bareTarget)` interprets the target as a class number in the child's project.
- Repos with zero registry classes (hotcwap, api-haven today) register nothing; renumbering a repo's `*-type.h` is its own owner-side feature commit (the Git Workflow Law), and downstream consumers of changed class numbers are updated upstream-first (`vexspoke` → `graphvex`/`api-haven`/`hotcwap`/`darling-framework`).

---

## 8. Canonical Include Paths Law (Zero Parent Hops)

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

---

## 9. Standalone Autonomy Law (Target Seam)

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

---

## 10. Living Documentation Law

A document is living only when its description changes alongside the code or contract it describes. Three surfaces are maintained together: source-file blueprints, canonical/repo-local preferences, and feature-readiness records. Preserve each surface's requirements below; calling a document living does not waive its details.

### Source-file blueprints (`;;OVERVIEW` and `;;DEFINITION`)

`;;OVERVIEW` and `;;DEFINITION` are the documentation and structural blueprint standards. Every `.c` (and `.m` where applicable) must be self-contained so that a developer or AI agent can immediately understand the class, its memory layout, and all its capabilities from the first 100–150 lines of the implementation file without having to tab back and forth to the `.h` file.

#### Separation of Roles:
1. **`;;DEFINITION` — The Architectural Raison d'Être**:
   Precedes or accompanies the overview. Written in fluid, paragraphical prose rather than rigid bureaucratic forms. Its depth scales naturally with the actual complexity of the system:
   - For straightforward data structures or leaf adapters, a focused paragraph explaining its purpose, lifetime, and bounds.
   - For complex coordinators or supervisors, comprehensive paragraphs explaining architectural necessity, memory layout, operational mechanics, concurrency models, and relationship to adjacent subsystems (R1–R5).
2. **`;;OVERVIEW` — The Structural Summary & Public/Private Registry**:
   Serves as the machine-readable and human-scannable diagram of fields, helpers, and functions.

#### Readable, current class map:
Keep both `;;DEFINITION` and `;;OVERVIEW` for a class implementation. The definition explains why the class exists, its lifetime, ownership, and non-obvious invariants. The overview summarizes its own class fields in header order, identifies any private helper or slot record, and lists the actual public API and significant private behavior so a reader can navigate the file without reconstructing it. Identify what is exported by the owning `.h` and what is file-local `static`; omit empty categories. A concise constructors/core/setters/getters grouping is welcome where it helps, but eight mandatory registry headings, `- (none)` placeholders, separator art, and prescribed body banners are not required. Explain difficult paths where they live.

Keep the overview near the start of the implementation. When fields, constructors, or behavior change, update the relevant overview and definition in the same change. A stale field list or an API listed but no longer present is a defect. A pure procedural entry point with no owned class may use `MODULE:` instead of `CLASS:`; a file owning two public behavioral classes must split them under the Single Class Per File Law (Java Law). Existing useful detailed overviews are not removed merely because this minimum is shorter.

### Preferences (architectural invariants)

#### Definition:
`preferences.md` at the root of the `vexgraph` workspace is the supreme constitutional law and single source of truth for the entire multi-repo ecosystem (`hotcwap`, `darling`, `vexspoke`, `graphvex`, `api-haven`).

#### The Why:
In a multi-repository workspace consisting of independently versioned C and native libraries, architectural entropy and convention drift are fatal. If rules live only in developer memory, chat histories, or scattered READMEs, rules will be contradicted and broken within days. A system with zero GC and manual memory layouts requires absolute, unbroken alignment across all subsystems.

#### The Rule:
1. **Same-Cycle Update & Local Commit**:
   Whenever an architectural invariant, convention, rule, or preference is introduced, modified, refined, or clarified, `preferences.md` must be updated and locally committed in the same development cycle. Out-of-date preferences are an architectural defect.
2. **Universal Reference Link**:
   Every sub-repository must include a `CONTRIBUTING.md` that explicitly links back to `vexgraph/preferences.md` as its supreme guiding authority.
3. **Subsystem Conformance**:
   Every implementation across `hotcwap`, `darling`, `vexspoke`, `graphvex`, and `api-haven` must adhere strictly to the laws codified herein. No repository is exempt.
4. **Title-Identity Enforcement**:
   Because this document is living, no law may be cited by its number anywhere — in code, in docs, or in git history — only by its canonical Title per the Law Identity Doctrine. Any stale numeric citation is a defect to fix in the same cycle it is noticed.

### Feature readiness (status and proof)

#### Definition:
Each repository's feature readiness matrix lives in the ecosystem wiki repo (`../../_repositories/.ecosystem/<repo>.md`, rendered as the `[[<repo>]]` wiki pages), one row per feature (container/widget/module/command), each carrying a scope line and a status emoji. The matrix is a **living inventory**, not a snapshot: its status column is the machine-readable handshake the ecosystem uses to know what is real vs stubbed vs absent.

#### The Why:
Multi-repo ecosystems rot silently — a header-only dialog or a half-stubbed picker looks "implemented" from the call site until someone depends on it and hits the empty paint. A single, always-current matrix — one row per unit, read by machines and humans alike — makes build-readiness legible at a glance, keeps scope lines honest, and exposes the next structural wedge (the largest contiguous 🟥 block) the moment it appears.

#### The Rule:
1. **Same-cycle status law, per file pair.** Any commit that ships, stubs, retires, or re-scopes a feature **must move its `../../_repositories/.ecosystem/<repo>.md` row in the same cycle** — code commit first, wiki row-write immediately after, never a deferred "update checklist" blob (the Git Workflow Law). Code and wiki live in different repos so they ship as separate per-repo commits, but a green-on-disk row that is stale-red on the sheet is still a broken intermediate state.
2. **Status legend (canonical, mirrors the wiki `Home.md` Status Legend):** 💚 98% done, production-ready · 🟩 95% done, implemented & functional · 🟨 85% done, substantially implemented · 🟧 75% done, partial/draft · 🟥 concept/draft, zero working source · ⬜ vital future work, not implemented (⬜ is never "dropped/archived"; it marks an important concept not yet built). A row's scope line is rewritten when the actor or deliverable changes, not just the emoji.
3. **Test proof gates the status.** 🟨 rows carry test names in the scope column (`tests/<name>_test`); a row is never 🟩 before its unit tests pass under `-Wall -Wextra -Werror` (the Git Workflow Law). Moving a row up without its proof is inflation; use the Conflict Triage Law (`;;INTENTION`) instead of silently overstating.
4. **Commits are per-checklist-file, per-repo.** The matrix lives as one row-write inside its feature commit; cross-repo rows never bundle (the Git Workflow Law). Code and wiki ship as separate per-repo commits in the same cycle — the code commit carries the behavior, the wiki commit carries the row.
5. **The spearhead is the wedge, not the tail.** The next work item is chosen as the structural keystone that unblocks the largest contiguous block of 🟥 rows (e.g. `OverlayRoot` unblocking the dialog/dropdown family), then the block collapses down the matrix — mirrors the upstream-first law (the Git Workflow Law).

### Legacy title map

Older citations of *Living `;;OVERVIEW` & `;;DEFINITION` Blueprint Law*, *Living Preferences Law*, and *Living Feature Readiness Law* refer to the corresponding clauses of the Living Documentation Law. New citations use the current Index title.

---

## 11. Bounded Wait Law (No Unbounded Waits on Joined Threads)

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

## 12. AI-First Architecture Manifesto Law

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

## 13. Conflict Triage Law — Managed Exception, Not Veto

### Definition:
When laws conflict, or intent outgrows a law, the answer is never a bare "this violates X." It is "unless you want it, here is how we manage it." The thought prevails; the laws adapt in the same cycle per the Living Documentation Law (Preferences).

### The Why:
A veto-only system freezes ambition (multi-app Kernel, R1–R5 ecosystem, 30 grammars, game engines). Tier 1 exists to prevent crashes, not to prevent thinking. Every conflict is triaged, given a managed path, and codified so the next agent inherits the decision.

### The Protocol:
1. **Name the tiers:** Tier 1 (crash/leak/deadlock/memory/thread safety) beats Tier 2 (model/contracts) beats Tier 3 (syntax). State which tier each conflicting law lives on.
2. **Assess before blocking:** state applicability first — does the law actually cover this case (link-time vs runtime, single-app vs Kernel multi-app, global vs per-arena)? A misapplied law is not a violation.
3. **Managed exception:** propose the indirection that preserves Tier 1 while granting intent. Canonical moves: opaque handle + callbacks instead of downstream `#include` (keeps the Vertical Integration Law / Standalone Autonomy Law); fixed array + count + getter instead of `**` chains (keeps the Semantic Consistency Law (Access depth)); `MemoryArena_create/freeAll` + bounded-join instead of globals (keeps the Vertical Integration Law (Teardown) / Bounded Wait Law); `;;INTENTION("reason")` + `;;DRAFT` markers for Tier 2/3 waivers.
4. **Prefs patch in-cycle:** if intent prevails, draft the exact `preferences.md` wording change now. Tier 1 waivers additionally require an alternate safety proof (no unbounded wait, no use-after-free, no circular link) reviewed heavily. Tier 2/3 waivers require `;;INTENTION` + overview/docs update in the same commit.
5. **Never silent drift:** a managed exception without its prefs + overview + docs update is a defect, same as stale prefs under the Living Documentation Law (Preferences).

---

## 14. Cold-Strict, Hot-Minimal Validation Law (Crash-Guard Split)

### Definition:
Validation splits by path temperature. The Tier-1 crash-guard half: no function
ever crashes, blocks unboundedly, allocates, or use-after-frees on null,
out-of-bounds, overflow, cancelled, or timed-out input — it returns `false` or
a Single Class Per File Law (Java Law) symmetric-accessor safe default instead. The Tier-2
contract half: setters validate at least as strictly as getters, with the
reject-or-clamp policy stated in the `;;OVERVIEW`; getters return safe
defaults per the Single Class Per File Law (Java Law).

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
   integer overflow, cancelled, timeout. One `THROW(...)` per failure at most
   (the THROW Law), then drop-degrade per the Bounded Wait Law (return
   `false`, keep old content, move on).
2. **Hot paths guard minimally, never log.** Vk present, `Raster`, `SdfGpu`,
   darling layout, `GfxLoop_frame`, `presentFrameLocked`: at most one `nullptr`
   entry guard returning `false`, zero per-element revalidation, zero logging,
   zero allocation. The hot path trusts the cold-validated handle. Deeper
   invariants are proven at compile time (`_Static_assert`) or tested at the
   owning seam, never assumed safe merely because an annotation exists.
   Optional debug probes may compile out in Release; checks needed to prevent
   out-of-bounds access, use-after-free, or corrupt input do not.
   ```c
   if (self == nullptr)
       return false;
   ```
3. **The Truncation-Never-Silent clause.** Copying into a bounded buffer takes
   `(src, dest, destCap, outTruncated)` — dest-last per the Semantic Consistency Law (Argument order),
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

---

## 15. Data-Oriented Storage Law & Object-Oriented Ergonomics

### Definition:
Collection patterns (nodes, lists, tables, trees) use flat, index-based
data-oriented backing storage while exposing an object-oriented class API.
The Single Class Per File Law (Java Law) owns accessor rules and the Semantic
Consistency Law owns naming and argument order; this law owns their combination
with flat storage.

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
2. **API is object-oriented.** The public class API provides the operations
   consumers need without exposing backing indices or forcing them to pierce
   internals. Accessors follow the Single Class Per File Law (Java Law), and
   naming and output order follow the Semantic Consistency Law. R4-specific
   part verbs belong to Darling's repo-local preferences, not this universal law.
3. **Keep application-specific class choices local.** Darling's
   `ExpandableListContainer`, checklist mode, and file-tree composition belong
   in its repo-local contract. They are examples of this storage/API split,
   not requirements on every collection across the ecosystem.

---

## 16. Test Segregation Law (Zero Source Pollution — No Tests in Source Trees)

### Definition:
Test code and harnesses NEVER reside inside production source directories (`src/`, `darling/`, `render/`, `../../trash/main/`, `app/`, etc.). Tracked test sources live in the independent workspace `tests/<subsystem>/` repository, or an owning repository's top-level `tests/` when built standalone. Production source trees contain only production classes, headers, and build scripts.

### The Why:
Colocating tests alongside production source files pollutes the clean 1:1 class-to-file architecture (the Single Class Per File Law), confuses directory-based build tools and file watchers, muddles static analysis, degrades search/grep ergonomics, and creates risks of circular dependencies or accidental linkage of test helpers into production shared libraries. A source directory must be purely production code; test suites are clients of the subsystems they test and must sit in segregated test directories.

### The Rule:
1. **Zero test files in production trees.** No file named `*_test.c`, `test_*.c`, `*_test.h`, `test_*.h`, or `*_demo.c` may ever be placed in or committed to a production source directory (`src/`, `darling/`, `render/`, `text/`, `event/`, `app/`, `hot/`, etc.). Violations must be rejected in review and failed in CI.
2. **Unified test hierarchy.** The independent tests repository uses `tests/<subsystem>/` (e.g., `tests/darling/`, `tests/vexspoke/`, `tests/graphvex/`, `tests/hotcwap/`, `tests/api-haven/`). Every release-relevant test must be built and registered with the runner; a file present on disk is not proof of execution.
3. **Subsystem partitioning.** Tests are grouped strictly by the subsystem they exercise:
   - `tests/darling/`: UI widgets, containers, text rendering, layout, and event dispatcher tests.
   - `tests/vexspoke/`: Core primitive, memory, threading, time, io, and relational tests.
   - `tests/graphvex/`: GPU buffers, rendering passes, texture, and font backend tests.
   - `tests/hotcwap/`: Hot reload, manifest parser, kernel, and window lifecycle tests.
   - `tests/api-haven/`: API client, webhooks, MCP server, SSE, and AI provider tests.
4. **Standalone repo test contract.** When built without the independent tests checkout, each owning repo may keep tracked tests in a top-level `tests/` directory, never inside production source folders.
5. **Track proof, ignore outputs.** Commit test source and build wiring in the appropriate repository; ignore generated binaries, scratch dumps, and build directories, never the test source tree itself. Release proof includes a test build that retains assertions and executes the registered negative and normal paths; sanitizer/crash suites supplement it rather than replacing runtime safety checks.

---

## 17. No Section Sign Law

### Definition:
The section sign (U+00A7) — the "double-S" — is forbidden everywhere: source comments, `;;OVERVIEW` blocks, docs, commit messages, wiki rows, and this document. Section references are always written as plain ASCII words: "see section 32", "the KeyMap section", "sections 41–48" — never the glyph-prefixed forms.

### The Why:
The glyph renders as an ugly double-S that reads as a typo in monospace, breaks `grep` for section references, and mangles in fonts, terminal pipelines, and localized tooling. The word "section" costs nothing and survives every tool, font, and copy-paste intact. A codebase that already bans arrow sugar for machine-readability has no business smuggling invisible punctuation into comments.

### The Rule:
1. **Never write the glyph.** New code, new docs, new commits: the character (U+00A7) is a defect on arrival, same as `->` under the Semantic Consistency Law (Reference form). Write "section" (or drop the marker) instead.
2. **Migration completed workspace-wide.** Every occurrence in the workspace was scrubbed in the same cycle as this update — source comments, test-section markers, `_docs/` and `_bugs/` notes, and this document. A reintroduced glyph is a defect on arrival (the Living Documentation Law (Preferences) zero-drift rule applies to this migration too).
3. **Canonical artifacts.** The occurrences present in `preferences.md` and the canonical docs at the time this law landed were scrubbed with it; the umbrella-local legacy markers now read as plain words (`// section N` test-section comments).

---

---

## 18. Per-Repo Preferences Extension Law

### Definition:
The central `preferences.md` codified in `vexspoke` serves exclusively as the universal supreme constitution, containing only the foundational invariants mandatory across all ecosystems and repositories. Individual repositories maintain their own standalone, self-identifying `<repo>-preferences.md` file at their repository root. Each per-repo preferences file carries the universal constitution plus any domain-specific laws that physically bind that repository's system level and responsibilities.

### The Why:
Monolithic constitutions force developers and AI agents working on isolated subsystems (e.g. GPU shaders, database persistence, or audio processing) to parse through dozens of irrelevant UI or windowing rules with no clear signal of which laws actually bind their work. Conversely, fragmenting rules without a central authority causes silent divergence and rule drift. Decoupling repo-local mirrors from the universal constitution ensures immediate clarity of local obligations while preserving universal invariants with zero drift.

### The Rule:
1. **Naming:** Every repository-local preferences file must be named `<repo>-preferences.md` (e.g., `graphvex-preferences.md`, `hotcwap-preferences.md`) located at the repository root. It never shadows or renames the universal `preferences.md`.
2. **Standalone Autonomy:** Each file sits physically at its own repository root and is never a symlink into `vexspoke`. Repositories checked out standalone remain fully self-describing.
3. **Local Definitions:** Each `<repo>-preferences.md` defines its repo-specific laws in full and inherits the universal laws through its Markdown link to this file. Repo-local law headers use their Titles (`### <Law Title>`) without numeric ordinals.
4. **Markdown Canonical Link:** Every repo-local preferences document links to the canonical `preferences.md` in its Constitution Link section. No edition or synchronization annotation is required for Markdown preferences documents.
5. **Zero Drift Same-Cycle Review:** Whenever the universal `preferences.md` changes, review affected repo-local preferences in the same development cycle; update their Markdown indexes or definitions when their local contracts change.
6. **Law Binding Matrix:** Each per-repo preferences file starts with a repo-local Law Index (Binding Matrix) listing its own rules, scope, and enforcement; universal laws are inherited by reference to this canonical Index and are not duplicated in that table.

---

## 19. toString Law (Every Object Has a String)

### Definition:
Every class across the ecosystem ships **two bounded string projections**:

1. **`Class_toString(self, dest, cap, outTruncated)`** — the **VALUE** string: a concise, class-specific summary of the object's state.
2. **`Class_toStringStruct(self, dest, cap, outTruncated)`** — the **STRUCTURE** string: a by-name dump of the object's own fields (**ONE layer only** — a nested object field renders via that object's `toString`, never by recursing into its `toStringStruct`).

Both are bounded (dest-last + a truncation flag) and cold-path only.

### The Why:
Every object has a string, and without a uniform contract each subsystem invents its own ad-hoc printing — un-greppable, un-cappable, and unusable by an agent. One uniform pair makes state legible everywhere (a debugger's `po`, a log line, an agent's context). And the struct dump **mirrors the `;;OVERVIEW` STRUCT FIELDS** block, so the two enforce each other: a field added without updating the dump is a defect, exactly like a stale overview.

### The Rule:
1. **Every public class ships both.** No class is string-blind.
2. **Fixed signature:** `(const Class *self, char *dest, size_t cap, bool *outTruncated)` — dest-last (the Semantic Consistency Law (Argument order)), bounded, truncation flagged (the Cold-Strict, Hot-Minimal Validation Law).
3. **Null-safe:** a null `self` writes `"nullptr"`, never crashes.
4. **Cold-path only:** never called on a frame — it formats.
5. **`toStringStruct` mirrors the `;;OVERVIEW` STRUCT FIELDS** — same fields, same declaration order.
6. **ONE LAYER, no recursion.** A struct dump prints only this class's fields; a nested object field renders via its `toString`. Depth is bounded by design (no recursion guard needed); a class that wants a deeper view calls the child's `toStringStruct` itself.
7. **Escape:** string fields are emitted quoted and escaped (`\n`, `\t`, `\"`, `\\`).
8. **Buffer only:** the bounded form is the law; there is no heap `toStringAlloc`.
9. **The formatter fuses here:** the Label formatter's `{object}` placeholder calls `toString`.

---

## 20. Authorial Intent Law

### Definition:
Deliberate author decisions — magic sentinels, sugar macros, naming choices that look surprising on first read (`SIZE_AUTO` as the FourCC `åuto`, `VEX_*` sugar, and their kin) — are documented at their definition site as intentional acts of the author, vex. The stamp is a plain comment, never a `;;` annotation macro:

```c
// INTENTIONAL(vex): SIZE_AUTO is the FourCC "åuto" (0xE575746F) by design —
// negative on every platform so any negative dimension reads as AUTO.
```

`;;INTENTION("reason")` keeps its existing meaning under the Conflict Triage Law: why a waiver or managed exception exists. `// INTENTIONAL(vex): ...` means something narrower: this strangeness is not a bug, not AI drift, not a placeholder — the author chose it and wants it preserved.

### The Why:
An AI pair-programmer writes most of the boilerplate in this ecosystem, so a future reader (human or agent) cannot tell a deliberate aesthetic from an accident. Without a provenance stamp, the next passer-by "fixes" the magic: normalizes the FourCC to a round number, renames the sugar to something bland, refactors away the joke that was actually a contract. The stamp draws a line around the author's deliberate weirdness and says: this survived review, it is load-bearing taste, leave it alone unless vex says otherwise.

### The Rule:
1. **Plain comment, fixed shape.** The stamp is exactly `// INTENTIONAL(vex):` followed by prose. No `;;AUTHOR`, no `;;DELIBERATE`, no new header in `src/annotation/` — the Two-Semicolon Annotation Style Law is untouched. The shape is greppable: `INTENTIONAL(vex):`.
2. **Definition site, not call sites.** One stamp where the decision is defined (the macro, the sentinel, the canonical name). Call sites stay clean — they just use it.
3. **What + why, one breath.** Each stamp states what the decision is and why it is that way (platform behavior, readability, contract). A stamp with no reason is a defect — restate or remove.
4. **Not a waiver.** Authorial intent never overrides Tier 1. A deliberate decision that risks a crash, leak, deadlock, or unbounded wait still goes through the Conflict Triage Law with its `;;INTENTION` + safety proof. `INTENTIONAL` documents taste; `INTENTION` justifies exceptions.
5. **Retrofit as noticed.** Existing deliberate decisions (`SIZE_AUTO`, `VEX_*` sugar) gain their stamps when touched or noticed, same cycle — no separate migration blob. New deliberate decisions ship their stamp in the same commit as the decision itself, per the Living Documentation Law (Preferences).

---

## 21. No Hardcoding Law

A constant used to initialize a value is a **default**, not a permanent user-visible decision. Name it at its owner, allow the consumer to change the current value through a validated interface, and read the current value at use sites. Never treat a named default as proof that the resulting value is final.

### Defaults and final constants

- Configurable sizes, thresholds, delays, colors, counts, cadence, and layout choices start from named defaults (`SCROLL_BAR_THUMB_MIN_DEFAULT`), then use current state that an application or user can adjust. Validate updates and expose current state through the class API.
- A value may be final only when its meaning is inherently fixed by math, ABI, wire protocol, physical format, or a justified safety/security bound. State that reason at its definition; do not pretend a configurable preference is one of these constants. Changing an external-input safety bound requires an explicit, safe validation policy, not unchecked setter access.
- Replace non-obvious bare literals with named values; numeric constants used by construction follow the Semantic Consistency Law (Construction and arity). Runtime-varying values come from their source of truth, not a cached guess.

### Growable work and explicit bounds

- Dynamic entity collections, registries, children, layers, and task queues use current counts and growable storage. A named initial capacity is not a rejection ceiling. Growth may happen on a cold path so hot paths can remain allocation-free.
- Fixed protocol, hardware, or safety limits are permitted only when documented at their owner, exhaustion is observable and safe, and tests prove the boundary. Never silently ignore excess work or impose a guessed limit on dynamic entities.
- Layout geometry derives from parent extents and the current configuration; continuous resize must not be disabled merely to avoid the general case.

### Time and deterministic motion

- Cadence comes from the display, caller clock, or another explicit source; never assume a fixed frame rate. Motion is a function of elapsed time, not frame count. Headless tests pass explicit time and named magnitudes so the same elapsed time produces the same result.

### Legacy title map

*Dynamic Scalability & Anti-Hardcoding Law* and *No Hardcoding Law* refer to this unified No Hardcoding Law. New citations use the current Index title.

---

## 22. WHAT Law

### Definition:
A `void*` is a promise with no receipt. The WHAT Law is the receipt: every
`void*` whose pointee type is not self-evident from its own name carries a
`;;WHAT("<type>")` annotation on the line above its declaration, naming exactly
what it points at. The type text is a string literal in C form —
`;;WHAT("uint64_t")`, `;;WHAT("Reactive")`, `;;WHAT("Field")` — so the pointee
is stated once, in place, and never inferred.

### The Why:
The relational engine runs on `void*` — everything is a pointer, and the same 8
bytes may hold a scalar, a struct, a reactive, or a table. An unnamed pointee is
the single largest source of guessing for the reader: the human author six
months later and the AI agent holding the file in context both re-derive a type
the code already knew when it was written. The runtime already carries a 16-byte
self-describing header (the Self-Describing Memory Block Law); the WHAT Law is
its **compile-time twin**, stating the intended pointee at the declaration site
so intent is legible before a single byte is allocated. It is to declarations
what the `toString` Law's struct dump is to live objects — a self-describing
contract that keeps the file honest.

### The Rule:
1. **Every non-obvious `void*` declares its pointee.** A `void*` whose type is
   not evident from its name (`health`, `payload`, `handle`) carries
   `;;WHAT("<type>")` on the line above. A `void*` already named for its type
   (`messageBytes`, `vertexData`) need not.
2. **Line-above form only.** The marker is a `_Static_assert` (the
   Two-Semicolon Annotation Style Law) and therefore an annotation line:
   `;;WHAT("uint64_t")` above `void* x;`. There is no inline
   `void* WHAT("...") x;` — an annotation is not a declarator.
3. **The text names the C type.** `uint64_t`, `Field`, `Reactive`,
   `Reactive<uint64_t>`, `char*` — the same spelling a cast would use.
4. **Agrees with the runtime header.** A documented `;;WHAT("T")` and the
   block's `Memory_type()` should agree; a mismatch is a defect (the
   Self-Describing Memory Block Law's zero-secondary-storage rule).
5. **Tier 2, zero cost.** The law binds the contract, not the hot path — the
   marker is `_Static_assert(1, ...)` and compiles to nothing.

---

## 23. Cold-Only Reflection Law

### Definition:
The Relational Engine's reflective walk — resolving a name to a value, or
following a dotted path (`character.position.x`) node to node — is a **cold
rendezvous**. It runs when a human or a tool asks "where is this thing, right
now?": a search box, a debugger, a script binding, a save/load walk, a hot-swap
rebind, telemetry. It must **never** run on a per-frame or otherwise hot path. A
hot path that resolves by name, chases the node graph, or re-walks the shelf is
a defect — the same weight as a lock or an allocation on the frame path.

### The Why:
The walk is pointer-chasing by design (it is a graph), and pointer-chasing is
the one thing a hot loop must not do: every hop risks a cache miss, and the
node graph is deliberately node-shaped rather than flat. The engine already
draws this line — hot iteration stays data-oriented and flat (sweeping the
`ChunkedList`), and cold rendezvous comes to the Relational Engine. Reflection
is the cold side. Mixing them costs frames: a name lookup per entity per frame
turns a cache-friendly sweep into thousands of pointer hops, and the cost scales
with the scene, not with the work. Keeping the walk cold keeps the hot path
flat, predictable, and bounded.

### The Rule:
1. **Reflection is cold-only.** Name resolution, dotted-path resolution, and any
   node-graph walk run on cold paths (frame-invariant work: setup, search,
   debug, script, save/load, hot-swap, teardown). Never per frame, never per
   entity per tick.
2. **The marker is explicit.** A module that hosts a cold resolver carries
   `;;INTENTION("cold path search is a node walk by design; hot iteration stays
   DOD")` at the walk site, so a future reader sees a chase and reads *why*
   rather than "fixing" it into a hot loop.
3. **Hot paths stay flat.** Hot per-frame iteration and per-frame field access
   use flat, data-oriented storage (the Data-Oriented Storage Law) and hoisted
   locals (the Semantic Consistency Law (Access depth)) — never a reflective walk.
4. **A hot reflective call is the defect, not the chase.** If a resolver shows
   up on a frame path, the call site is wrong, never the resolver's design.
5. **Resolve once, hold the pointer.** A hot consumer that needs a value
   repeatedly resolves the name (or path) once, on the cold path, and holds the
   resulting pointer — it never re-resolves per frame.

---

## 24. Install Ledger Law (Machine-Scoped Install Memory)

### Definition:
The install ledger is a machine-scoped record of the application identities
(`org`/`app`) that have been installed on this host. It lives OUTSIDE the
install tree, in the OS per-user state directory as a plain record file
(macOS `~/Library/Application Support`, Windows `%LOCALAPPDATA%`, Linux
`$XDG_STATE_HOME`). `MANIFEST_REFLECT` records an install; `UNINSTALL` removes
the tree but keeps the record (state `uninstalled`); and `MANIFEST_IS_FIRST_RUN`
consults the ledger, so a wiped tree is never a fresh install. The in-tree
`manifest.json` remains the ownership marker for safe deletion (see the
Manifest Resilience Law); the ledger is the machine's out-of-tree memory of the
install.

### The Why:
The in-tree `manifest.json` proves a directory is ours to delete, but it dies
with the tree. Without an out-of-tree record, deleting the install directory —
or just its fingerprint — makes the machine report a fresh install again,
losing the upgrade, migration, and re-install state every platform installer
must retain. A ledger that outlives the tree makes "has this app ever been
installed here?" answerable even after a full uninstall.

### The Rule:
1. **Out of tree.** The ledger never lives inside the install root; it is a
   machine-scoped store keyed by `org`/`app`.
2. **Per-user state file, outside the tree.** It lives as a plain record file
   in the OS per-user state directory (macOS `~/Library/Application Support`,
   Windows `%LOCALAPPDATA%`, Linux `$XDG_STATE_HOME`), never inside the app
   tree and never in the OS secure store or registry. It is a plain record, not
   a secure store.
3. **Uninstall keeps the record.** `UNINSTALL` clears the tree and flips the
   record to `uninstalled`; the record is never erased by uninstall. Only an
   explicit `forget` (a deliberate machine reset) purges it.
4. **First-run consults the ledger.** `MANIFEST_IS_FIRST_RUN` is true only when
   the ledger holds no record for the identity and the in-tree mark is absent.
5. **Correctness, not security.** The ledger answers "does the machine
   remember?", never "can the machine be stopped from forgetting?". A local user
   with the machine can always clear the store; the ledger must never be relied
   on as a security boundary.

---

## 25. Platform Support Floor Law (Apple Silicon macOS 14+, Windows 10+)

### Definition:
The ecosystem declares a minimum supported build/runtime baseline, not a ceiling on features. On macOS the baseline is **Apple Silicon** `arm64` M1 and **macOS 14.0 (Sonoma)**; on Windows it is **Windows 10**. Builds below that baseline are unsupported. Newer CPU, GPU, and OS features are independently gated at runtime under the Capability Gating Law.

### The Why:
macOS 27 "Golden Gate" is the first macOS to run exclusively on Apple Silicon; carrying `x86_64` paths or pre-14 shims buys nothing but doubles the code paths and slows every build. And a floor is the only thing that makes availability drift visible: the compiler warns about an unguarded newer API only when a deployment target is set, so "it compiled" becomes a promise that it runs on the oldest supported Mac.

### The Rule:
1. **macOS hardware floor: Apple Silicon (`arm64`), never Intel.** `CMAKE_OSX_ARCHITECTURES=arm64`. No `x86_64`, no `arm64;x86_64` Universal, no `#if defined(__x86_64__)` paths.
2. **macOS software floor: 14.0 (Sonoma).** `CMAKE_OSX_DEPLOYMENT_TARGET=14.0`. Every API newer than the floor is reached only through an explicit availability guard; an unguarded newer-API call is a defect, not a warning.
3. **Windows floor: Windows 10.** `_WIN32_WINNT` / `WINVER` pinned to `0x0A00`.
4. **Portable CPU baseline, never `native`, in shipped code.** Targets compile with `-mcpu=apple-m1` so a binary built on an M6 / A18 still runs on an M1. `native` is a local-dev-only convenience and is never the default.
5. **Platform-exclusive backends state their floor.** A backend compiled only on its own host records its proven floor and its unproved-on-other-host status (the Per-File Battle Test Law's explicit gap).
6. **The floor moves only by law.** Raising the floor — a new macOS number, a new chip family, a new Windows floor — is a deliberate amendment of this law, landed in the same cycle as the CMake change (the Living Documentation Law (Preferences); Zero Drift).
7. **A feature above the floor is a capability, not a floor move.** A hardware or OS feature newer than the floor — ray tracing on M3+, mesh shaders, an instruction set, a post-floor API — never raises the floor; it is a runtime-probed capability under the Capability Gating Law.

---

## 26. Capability Gating Law (Runtime Features Above the Floor)

### Definition:
The Platform Support Floor Law fixes the minimum the ecosystem is built for. Every capability **newer than that floor** — a GPU feature (ray tracing on M3+), a CPU instruction set (dot-product, SVE/SME), an OS API past the deployment target — is a **runtime-probed capability**, never a floor change. A capability is probed **once at cold boot**, cached in a bitset, and either used or replaced by a **stated fallback**; a capability a component **requires** is declared in the manifest and refused at the manifest gate on unsupported hardware.

### The Why:
Raising the floor to adopt a feature exiles every machine below it: hardware ray tracing arrived with M3, so "RT means floor M3" would drop the entire M1/M2 base. The floor answers *what do we ship*; a capability answers *what can this machine do*; conflating them throws away reach for every new feature. And the probe must be cold and cached — re-querying device features per frame is how frames die (the Cold-Strict, Hot-Minimal Validation Law).

### The Rule:
1. **The floor is the compile minimum; a feature is a capability.** A capability never moves the floor (the Platform Support Floor Law rule 7). RT, mesh shaders, SME/SVE, the Neural Engine, and any post-floor OS API are capabilities.
2. **Probe once, cold, cached.** Each capability resolves once at boot (device / CPU / OS probe) into a bitset; hot paths branch on the cached bit and never re-probe (the Cold-Strict, Hot-Minimal Validation Law; the Hot-Path Minimal Guard Law).
3. **Every capability has a disposition, and it is one of two.** *Best-effort*: a stated fallback path (RT absent → the Raster dialect). *Required*: declared in the manifest and refused at the manifest gate on unsupported hardware (the Dynamic Module ABI Verification Law's gate), with the reason surfaced (the Failure Observability Law). A silently degraded **required** capability is a defect.
4. **No capability is assumed.** A call site that depends on a capability checks the cached bit first; an unguarded use of a post-floor feature is the same defect as an unguarded post-floor API under the Platform Support Floor Law.
5. **CPU capabilities dispatch a variant; GPU capabilities select a dialect.** A newer CPU feature is reached only through a runtime-dispatched variant (`-mcpu=apple-m1` compiles the floor; the newer arm runs only when its bit is set). A GPU capability selects the dialect/pipeline (graphvex's Vulkan/Raster/Null rows).
6. **The probe is owned, named, and queryable.** Host / CPU / OS capabilities live in the R1 host (`Capability`); device / GPU capabilities live in the R3 driver (`graphvex`). Every capability has a stable name and a `has` query (the Single Class Per File Law (Java Law); the toString Law).

---

## 27. THROW Law (Loud Cold Rejection)

### Definition:
THROW is the recoverable diagnostic a **cold**, detected rejection uses to report itself. `THROW(fmt, ...)` (`exception/throw.h`) writes `[vex] <file>:<line>: <message>` to stderr; the caller returns its safe error/default. It does not unwind or terminate. Stderr output is synchronous and may block: never use this reporter in a hot loop, signal handler, or teardown path requiring a bounded wait. A segfault before the check cannot be reported by THROW; tests and sanitizers must detect such failures.

### The Why:
A silent no-op and a detected rejection are indistinguishable unless something reports which happened. THROW makes a cold rejection legible at its call site; it does not diagnose arbitrary crashes. Tests must assert both the expected error result and diagnostic, and normal release flows must not unexpectedly reach a THROW site.

### The Rule:
1. **Cold seams reject loudly.** A function that refuses input it cannot serve returns its safe default **and** calls `THROW(...)`, once per failure (the Cold-Strict, Hot-Minimal Validation Law). It never returns the default silently.
2. **Hot paths never THROW.** A `;;HOTCODE` getter, or any per-frame path, carries no THROW; its rejection is a silent safe default (the Hot-Path Minimal Guard Law). Observability is a **cold** obligation.
3. **One primitive, one format.** Rejections go through THROW only — never a bare `fprintf`, `printf`, `perror`, or ad-hoc print. The output is exactly `[vex] <file>:<line>: <message>`, so the channel stays greppable and machine-parseable.
4. **THROW does not unwind.** It never `longjmp`s or `exit`s. It reports synchronously, then the caller returns a safe result; do not claim stderr is nonblocking or allocation-free. A distinct, explicitly fatal host operation owns any process-terminating teardown.
5. **THROW is not a control path.** It marks a problem; it does not catch or resume. Catching, escalation, and any terminal disposition belong to the R1 supervisor (the Vertical Integration Law). The relational model has no exception stack.
6. **A THROW site is greppable and owned.** `grep -rn THROW` lists recoverable rejections; an owning test checks the emitted diagnostic and safe return. Normal-path tests must fail on unexpected diagnostics. Sanitizer and crash tests catch faults that never reach THROW.

---

## 28. Native Pixel Law

### Definition:
Native hardware display pixels are the single, absolute, universal metric across the entire ecosystem. Windows, frames, swapchains, render targets, boards, and mouse/touch input events speak exclusively in **native physical hardware pixels** on the target display. Virtual or platform-dependent coordinate spaces (such as Apple AppKit "points" or Windows DPI-virtualized units) are strictly encapsulated at the platform boundary: any platform layer that requires points must perform the conversion internally, preserving 1:1 hardware pixel determinism across all platforms (macOS, Windows, Linux/Wayland).

### The Why:
Tesler's Law (the Conservation of Complexity) + Cross-Platform Determinism.
On Retina and HiDPI displays, abstracting window geometry into logical points obscures physical reality: requesting an 800x600 window on a 2x Retina screen silently allocates a 1600x1200 hardware surface, leading to 4x memory overhead, visual distortion, and de-synchronization between mouse positions and GPU raster buffers. Game engines, graphics pipelines, and spatial interfaces require exact physical screen coordinates. By enforcing physical pixels as the universal currency, `Window_setSize(ptr, 800, 600)` creates a window of exactly 800x600 hardware pixels on the developer's display, and mouse events map 1:1 to rendered pixels.

### The Rule:
1. **Window Dimensions in Physical Pixels:** Window creation (`Window(...)`, `Window_create`, `WindowDesc`) and sizing (`Window_setSize`, `Window_width`, `Window_height`) operate strictly in native hardware pixels. Platform-exclusive backends (e.g. AppKit on macOS) must divide or multiply by `backingScaleFactor` internally at the OS boundary.
2. **Input Events in Physical Pixels:** All pointer and touch coordinates (`Mouse_pushMoveEvent`, `Mouse_pushDragEvent`, `Touch_*`) are delivered in native hardware pixels relative to the window's content top-left.
3. **Presentation Seam & Drawables:** Seam layers (`CAMetalLayer.drawableSize`) and Vulkan/Metal render targets match the native pixel size 1:1.
4. **Multi-Monitor Scale Transitions & Revalidation:** When a window transitions across monitors with differing backing scale factors (DPI), `Window_revalidate` recalculates the underlying platform points to preserve the exact physical pixel size and dispatches geometry update events.
5. **Points-Explicit Escape Hatch:** When logical points are explicitly needed for OS-specific desktop placement, classes provide explicit points accessors (`Window_setSizePoints`, `Window_getSizePoints`, `Window_widthPoints`, `Window_heightPoints`, `Window_getScale`).

