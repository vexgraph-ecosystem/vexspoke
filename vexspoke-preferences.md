# vexspoke — Repo-Local Living Preferences
> Repo-local preferences governed by the Living Documentation Law.
> Universal Supreme Constitution: preferences.md (vexspoke).

## 0. Constitution Link (supreme)
- [preferences.md](https://github.com/vexgraph-ecosystem/vexspoke/blob/main/preferences.md) (canonical, vexspoke) — accessible locally at ../../preferences.md
- All universal laws in `preferences.md` are mandatory and binding across the ecosystem.
- This document codifies **exclusive** preferences that apply uniquely to `vexspoke` (R2 Relational Memory Substrate).

## 1. Repo-Local Law Index (Binding Matrix)

Universal laws are inherited from the canonical `preferences.md` Index; this table indexes the additional laws specific to this repository.

| Law Title | Scope | Enforcement |
| :--- | :--- | :--- |
| **Coordinate-Agnostic Vector Law** | R2 Relational Memory Substrate | Mandatory for `vexspoke` |
| **Self-Describing Memory Block Law** | R2 Relational Memory Substrate | Mandatory for `vexspoke` |
| **BitPool Slot Segregation Law** | R2 Relational Memory Substrate | Mandatory for `vexspoke` |
| **24-Byte Variable Slot Law (The 23+1 Rule)** | R2 Relational Memory Substrate | Mandatory for `vexspoke` |
| **Reactive Generics Law** | R2 behavior (the reactive engine + the generic family) | Mandatory for `vexspoke` |

## 2. Exclusive Repo-Local Laws (FULL PROSE RESTATEMENT)

### Coordinate-Agnostic Vector Law

#### Definition:
In `vexspoke`, spatial vector representations (`Vec2`, `Vec3`, `Vec4`) must not be hardcoded to arbitrary Cartesian conventions (such as assuming `+Y` is always up, or that `Z` is depth, or that indices `0, 1, 2` must strictly mean `X, Y, Z`). Instead, vectors embrace coordinate-agnostic spatial semantics:
- Components are defined using anonymous unions providing semantic spatial terms: `horizontal` (Axis 0: left/right), `vertical` (Axis 1: down/up), and `depth` (Axis 2: back/front), alongside `w` for homogeneous coordinates.
- Directional getters and setters (`Vec4_getRight`, `Vec4_getUp`, `Vec4_getFront`, `Vec4_getLeft`, `Vec4_getDown`, `Vec4_getBack`) access components according to spatial meaning.
- Coordinate frame mappings are resolved via `CoordFrame` (such as `COORD_FRAME_Y_UP_LEFT` or `COORD_FRAME_Z_UP_RIGHT`), allowing one vector to project into any engine convention (Unity, Unreal, Vulkan, OpenGL) branchlessly without converting or re-baking storage.
- Storage defines `xyzw` and `rgba` as anonymous union aliases for the underlying 16-byte SIMD layout, so callers working in color spaces or 4D projective space have first-class, named access without casting.

#### The Why:
3D engines, graphics APIs, and UI frameworks constantly fight coordinate-system wars: Vulkan uses Y-down right-handed; OpenGL uses Y-up right-handed; Unity uses Y-up left-handed; Unreal and Blender use Z-up right-handed. Hardcoding `x, y, z` as immutable axes creates cognitive overload and brittle conversion code. By establishing a coordinate-agnostic representation centered on semantic axes (`horizontal`, `vertical`, `depth`, `w`) and aliasing them with `xyzw` and `rgba`, `vexspoke` serves as the universal mathematical substrate across all subsystems without taking sides.

#### The Rule:
1. **Semantic First:** Vector structs (`Vec2`, `Vec3`, `Vec4`) must provide semantic aliases (`horizontal`, `vertical`, `depth`, `w` or `right`, `up`, `front`) via anonymous unions.
2. **`xyzw` and `rgba` Parity:** `xyzw` is a first-class named alias in the union, alongside `rgba` for color vectors and `data[N]` for SIMD/stride access.
3. **CoordFrame Conversion:** Cross-subsystem orientation transfers must use `CoordFrame` basis queries (`Vec4_getXInFrame`, etc.) rather than ad-hoc negative signs or manual component swizzling.

---

### Self-Describing Memory Block Law

#### Definition:
Every memory block managed by `vexspoke` carries a self-describing bit-packed 16-byte header prepended before its payload pointer. The header encodes `type_id`, `length`, and allocation generation flags. Functions such as `Memory_type()` and `Memory_length()` decode these bits instantaneously with zero dictionary lookups.

#### The Why:
In a relational substrate where everything is a pointer, an opaque `void*` is hazardous unless the runtime can instantly discover its type, size, and validity. Bit-packed headers give every raw pointer self-describing introspection without requiring separate wrapper structs or runtime type dictionaries.

#### The Rule:
1. **Header Layout:** All blocks allocated through `Memory_alloc` reserve 16 bytes for header bits.
2. **Zero Secondary Storage:** Never store redundant length or class IDs in secondary hash maps when the pointer carries its own metadata.

---

### BitPool Slot Segregation Law

#### Definition:
Equal-stride data structures (primitives, relational rows, ring buffer cells, tokens) allocate strictly from parameterized `BitPool` pools. BitPools maintain ABA-safe generational bitmasks to vend slots with $O(1)$ allocation and deallocation without heap fragmentation.

#### The Why:
Dynamic general-purpose `malloc` degrades cache coherence and introduces non-deterministic latency. In the R2 core, memory must be predictable, linear, and cache-aligned.

#### The Rule:
1. **Fixed Stride:** Objects of identical size belong in a dedicated `BitPool`.
2. **Generational Tagging:** Freelist indices carry generational counters to prevent ABA hazards in lockless access.

---

### 24-Byte Variable Slot Law (The 23+1 Rule)

#### Definition:
Every interned variable name in `vexspoke` is strictly bounded to 23 ASCII characters plus a 1-byte NUL terminator (24 bytes total). Variable names are paired with an 8-byte intrusive self-pointer (`uint64_t self`) to form a cache-aligned, power-of-two 32-byte slot record (`StringSlot`: `[self 8B][name 24B]`). Two slots pack with byte-level perfection into a single 64-byte CPU cache line with zero padding waste.

#### The Why:
In a relational memory substrate where symbols resolve to addresses, string allocation must never cause heap fragmentation, cache-line thrashing, or indeterminate hashing latency. Unbounded string names lead to variable-stride records, secondary pointers, and cache misses. By pinning names to 23 ASCII characters ($3 \times \text{uint64}$ plus $1 \times \text{uint64}$ pointer), slots are strictly uniform (32 bytes), identity is stated once per process, and lookups execute via branchless binary search with direct 24-byte scalar compares. Names longer than 23 characters are rejected cold at the gate, because silent truncation would corrupt identity.

#### The Rule:
1. **Name Character Limit:** Variable and string pool names must be between 1 and 23 characters (`STRING_POOL_NAME_MAX = 23u`). Overlong names are rejected immediately.
2. **Exact 24-Byte Buffer:** The name buffer is exactly 24 bytes, NUL-terminated, and zero-padded.
3. **8-Byte Self Link:** Every slot reserves an 8-byte `self` pointer for $O(1)$ intrusive address validity checks.
4. **Zero Dynamic Allocation in Lookups:** Name resolution yields permanent slot indices (`int32_t`) that remain valid across table rehashes.

---

### Reactive Generics Law

#### Definition:
Reactivity is **one engine** and **one generic spelling**. `Reactive` is a single
type-agnostic engine — one atomic word, a shadow, a dirty flag, and the typed
channel lists — exposed through `Reactive(T)`: `Reactive(int)`, `Reactive(Vec4)`,
`Reactive(Buffer)`, where the argument is a leaf type. The typed classes are
**stamped, not hand-written**: `TYPEDEF_REACTIVE(NAME)` / `IMPLEMENT_REACTIVE(NAME)`
stamp any object in two lines, and the built-in families live one file pair each —
`reactive_primitive` (the scalar set), `reactive_object` (Vec2/3/4, Rectangle, the
pairs), `reactive_probable` (the merged probable). A `_Generic` surface
(`Reactive_set`/`Reactive_get`/`Reactive_addOnChanged`/…) names ONE function for
any type at the call site. Four channels carry typed callbacks: `onSet`,
`onChanged`, `onGet` (the one-fire read), `onNullptr`.

#### The Why:
The engine is already type-agnostic at runtime (one word, everything is a pointer),
so the generic layer is an **ergonomics and safety** surface, not a new runtime —
and generating it from templates means a new reactive is one line. The callback is
`fn(T value)` because the *value* is the payload: a consumer of "gold" hears the new
gold, not which reactive fired. Nesting is refused (`Reactive(Probable(T))` does not
exist) because the bell can only ring from **inside** the object that owns the value
— a reactive over a probable is the MERGED `ReactiveProbable(T)`, never a wrapper.

#### The Rule:
1. **One spelling.** `Reactive(T)`; the lowercase C spellings (`Reactive(int)`,
   `Reactive(float)`) bridge to the camelCase class. Never a bespoke per-type name
   at the call site.
2. **No nesting.** `Reactive(Probable(T))` does not exist; the merged
   `ReactiveProbable(T)` is the composite. A reactive over a probable pools belongs
   to the merged class too.
3. **Typed channels, `fn(T value)`.** The observer receives the value; `removeOn`
   matches the function-pointer address (there is no userdata). `onGet` is a
   one-fire read (fires with the value as it is read); `onNullptr` fires on an empty
   slot.
4. **One generic function surface.** `_Generic` resolves `Reactive_set`/`Reactive_get`/
   `Reactive_addOnChanged`/… to the per-class arm by the reactive's type; a bare
   `Reactive*` (or `nullptr`) routes to the raw engine channel.
5. **Families are one file pair each.** The scalar, object, and probable families
   each live in a single `reactive_<family>.{h,c}`; a *new* object is stamped by
   `TYPEDEF_REACTIVE(NAME)` on its own single-class file.
6. **Observers with no bind are free.** A metric with no observers must not fire on
   a hot path — a bare `Reactive_get` stays one atomic load.

#### Managed exception:
```c
;;INTENTION("the family files — reactive_primitive, reactive_object,
reactive_probable — waive the Single Class Per File Law BY INTENT: they are
mechanical template stamps that differ only in VEX_T, not hand-authored classes;
one file pair per family reads clearer than twenty identical ones. Per the
Conflict Triage Law.")
```

---

## 3. Repo-Local Extensions (managed, per the Conflict Triage Law)

;;INTENTION("R2 Relational Memory Substrate: bit-packed memory headers, coordinate-agnostic vectors, bitpool slot allocation, zero steady-state allocation.")

---

## 4. Readiness Cross-Reference (Living Documentation Law)

- Feature readiness matrix tracked in [`../../_repositories/.ecosystem/vexspoke.md`](../../_repositories/.ecosystem/vexspoke.md) (rendered as `[[vexspoke]]` wiki page).
