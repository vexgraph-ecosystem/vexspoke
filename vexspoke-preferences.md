# vexspoke — Repo-Local Living Preferences
> Repo-local preferences governed by the Living Documentation Law.
> Universal constitution: `gist.md` is the compressed law list; `PREFERENCES.md` is its rationale.

## 0. Constitution Link (supreme)
- [gist.md](https://gist.github.com/vex-graph/4132a6c45cb6d3797c3e8eff2e94035a#file-gist-md) — the compressed law list, mirrored in this workspace at `../../../metadata/gists/preferences/gist.md`. It is the only normative law text; `../../../metadata/gists/preferences/PREFERENCES.md` carries the rationale and is never ingested whole. Numbers are slice addresses inside one file, never law identity: cite by Title.
- All universal laws bind this repository. Read one on demand from the workspace root: `./tools/prefs law "<title>"`, `./tools/prefs section "<section>"`, `./tools/prefs repo <name>`.
- This document codifies **exclusive** preferences for `vexspoke` (R2 CPU computation, behavior, and network protocols). Relational Engine owns filesystem IO/NIO, the native Memory/MemoryArena/Transient allocator implementation, and the relational symbol table/spotlight runtime (`src/relational`), plus Rust storage and native C search. Vexspoke `net/` owns network protocol clients and servers, including BSD socket-backed HTTP. Existing C Memory ABI and type IDs remain compatible; this migration is not a Rust allocator rewrite.

## 1. Repo-Local Law Index (Binding Matrix)

Universal laws are inherited from the compressed constitution (`../../../metadata/gists/preferences/gist.md`); this table indexes the additional laws specific to this repository.

| Law Title | Scope | Enforcement |
| :--- | :--- | :--- |
| **Coordinate-Agnostic Vector Law** | R2 Relational memory Substrate | Mandatory for `vexspoke` |
| **Self-Describing memory Block Law** | R2 Relational memory Substrate | Mandatory for `vexspoke` |
| **BitPool Slot Segregation Law** | R2 Relational memory Substrate | Mandatory for `vexspoke` |
| **24-Byte Variable Slot Law (The 23+1 Rule)** | R2 Relational memory Substrate | Mandatory for `vexspoke` |
| **Reactive Generics Law** | R2 behavior (the reactive engine + the generic family) | Mandatory for `vexspoke` |
| **Pure Format Algorithm Law** | R2 CPU computation (`net/`, `search/`, `image/`) | Mandatory for `vexspoke` |

## 2. Exclusive Repo-Local Laws (FULL PROSE RESTATEMENT)

### Coordinate-Agnostic Vector Law

**Definition.** Spatial vectors (`Vec2`, `Vec3`, `Vec4`) are never hardcoded to a Cartesian convention: anonymous unions carry semantic spatial terms — `horizontal` (Axis 0: left/right), `vertical` (Axis 1: down/up), `depth` (Axis 2: back/front), plus `w` — and directional getters/setters (`Vec4_getRight`, `Vec4_getUp`, `Vec4_getFront`, `Vec4_getLeft`, `Vec4_getDown`, `Vec4_getBack`) access by spatial meaning; frames resolve via `CoordFrame` (`COORD_FRAME_Y_UP_LEFT`, `COORD_FRAME_Z_UP_RIGHT`, …), so one vector projects into any engine convention branchlessly; `xyzw`/`rgba` are aliases over the 16-byte SIMD layout. *Why:* engines fight coordinate-system wars; semantic axes keep vexspoke the universal substrate without taking sides.

_Rule._

1. Vector structs provide semantic aliases (`horizontal`, `vertical`, `depth`, `w` or `right`, `up`, `front`) via anonymous unions.
2. `xyzw` is a first-class named alias; `rgba` for color vectors, `data[N]` for SIMD/stride access.
3. Orientation transfers use `CoordFrame` basis queries (`Vec4_getXInFrame`, etc.), never ad-hoc negative signs or manual component swizzling.

---

### Self-Describing memory Block Law

**Definition.** Every block a Vexspoke consumer obtains through the engine-owned `Memory_*` ABI carries the existing 32-byte header (`typeId` with its `BE6C` sugar, `hash`, `length`, `pointer`) before its payload pointer; the owner moved to Relational Engine (ABI unchanged); `Memory_type()` and `Memory_length()` retain their existing behavior. *Why:* everything is a pointer; bit-packed headers make every raw pointer self-describing without wrapper structs.

_Rule._

1. All blocks allocated through `Memory_alloc` reserve 32 Bytes for header bits: the triple-sugar self-describe id, the recomputed hash veto over (id, length, pointer), the payload length, and the block's redirect.
2. The header validates two ways and fails closed on either — the `BE6C` sugar inside the id, and the hash over the stated fields; provenance beyond that is the arena-range gate, never a magic cookie.
3. The `pointer` word is the object's own address, or the value's address when the id's form bits (`Type_isPrimitive`) say the type is a primitive; the `BE6C` sugar answers that from the header alone.
4. A block is never relocated after allocation; any future arena compaction must re-stamp every moved header.
5. Never store redundant length or class IDs in secondary maps — the pointer carries its metadata.

---

### BitPool Slot Segregation Law

**Definition.** Equal-stride data structures (primitives, relational rows, ring buffer cells, tokens) carve their backing once from the engine arena (`MemoryArena_alloc`) and vend slots from a parameterized `BitPool`: ABA-safe generational bitmasks give $O(1)$ claim/return with no per-object allocator call. *Why:* slab locality keeps the R2 core predictable, linear and cache-aligned while the engine stays the single owner — a `BitPool` is a layout over engine bytes, never a second heap (the `calloc`/`free` in `src/bit/bit.c` today is a Relational Memory Law defect to retire).

_Rule._

1. Objects of identical size belong in a dedicated `BitPool`.
2. Freelist indices carry generational counters to prevent ABA hazards in lockless access.

### 24-Byte Variable Slot Law (The 23+1 Rule)

**Definition.** Every interned variable name is strictly bounded to 23 ASCII characters plus a 1-byte NUL terminator (24 Bytes total), paired with an 8-byte intrusive self-pointer (`uint64_t self`) as the cache-aligned, power-of-two 32-byte slot record (`StringSlot`: `[self 8B][name 24B]`); two slots pack byte-perfect into a single 64-byte CPU cache line. *Why:* the 23-byte pin keeps slots uniform, identity stated once per process, lookups branchless binary search over direct 24-byte compares — overlong names reject cold, silent truncation would corrupt identity. Backing comes from the engine arena (`MemoryArena_alloc` in relational-engine's `relational/variable_pool.c`); the engine's Rust `VariableSlot` is a different record (`[name 24B][value 8B]`) and the two are never conflated.

_Rule._

1. Variable and string pool names must be between 1 and 23 characters (`STRING_POOL_NAME_MAX = 23u`).
2. The name buffer is exactly 24 Bytes, NUL-terminated, zero-padded.
3. Every slot reserves an 8-byte `self` pointer for $O(1)$ intrusive address validity checks.
4. Name resolution yields permanent slot indices (`int32_t`) that remain valid across table rehashes.

---

### Reactive Generics Law

**Definition.** Reactivity is **one engine** and **one generic spelling**: `Reactive` is a single type-agnostic engine, exposed through `Reactive(T)` with a leaf-type argument. Typed classes are **stamped, not hand-written**: `TYPEDEF_REACTIVE(NAME)` / `IMPLEMENT_REACTIVE(NAME)` stamp any object in two lines; built-in families live one file pair each — `reactive_primitive`, `reactive_object`, `reactive_probable`. *Why:* the engine is already type-agnostic at runtime — the generic layer is ergonomics/safety and a new reactive stamps in one line; the callback is `fn(T value)` because the *value* is the payload, and nesting is refused because the bell can only ring from **inside** the value's owner.

_Rule._

1. `Reactive(T)`; the lowercase C spellings (`Reactive(int)`, `Reactive(float)`) bridge to the camelCase class. Never a bespoke per-type name at the call site.
2. `Reactive(Probable(T))` does not exist; the merged `ReactiveProbable(T)` is the composite. A reactive over a probable pools belongs to the merged class too.
3. The four channels `onSet`, `onChanged`, `onGet` (one-fire read) and `onNullptr` (empty slot) carry typed `fn(T value)` callbacks — the observer receives the value; `removeOn` matches the function-pointer address (no userdata).
4. `_Generic` resolves `Reactive_set`/`Reactive_get`/`Reactive_addOnChanged`/… to the per-class arm by reactive type; a bare `Reactive*` (or `nullptr`) routes to the raw engine channel.
5. A *new* object is stamped by `TYPEDEF_REACTIVE(NAME)` on its own single-class file.
6. A metric with no observers must not fire on a hot path — a bare `Reactive_get` stays one atomic load.
7. Owner-affine notification (inherited from the canonical constitution, R2 Reactive): `Reactive_set` never fires observers; the owner calls `Reactive_drain`, coalescing pending writes into one `onSet`/`onChanged` batch on the owner's thread.

```c
;;INTENTION("the family files — reactive_primitive, reactive_object,
reactive_probable — waive the Single Class Per File Law BY INTENT: they are
mechanical template stamps that differ only in VEX_T, not hand-authored classes;
one file pair per family reads clearer than twenty identical ones. Per the
Conflict Triage Law.")
```

### Pure Format Algorithm Law

**Definition.** Vexspoke owns **pure format algorithms** — byte-to-byte code with no file, path, handle, device or persistent state: the JSON parser (`net/json`), the URL parser (`net/url`), and the PNG → RGBA8 decoder (`image/png`, system zlib for DEFLATE); each takes a borrowed byte span and returns a value, none reads a file or opens a path. *Why:* decoding is computation, not storage — the *caller* owns I/O (the engine's `File`), *storage* stays with the engine, every tier above R2 borrows the same pure function, and decoding differs from image **composition** (Graphvex R3 uploads/samples, never decodes) and terminal **rasterisation** (Determinant R3).

_Rule._

1. A format algorithm takes `(const uint8_t *bytes, size_t length)` and returns a decoded value; it never opens a file or path.
2. Decoded buffers are allocated through the engine's `Memory_*` ABI; the engine owns the storage, and the caller frees its stable pointer with `Memory_free`.
3. Unsupported or hostile input returns a named code (`PngCode`-style) with one cold `THROW`; a corrupted stream never yields plausible output. Geometry is bounded before allocation.
4. Checksums/CRCs are validated (PNG chunk CRCs), and an inflate that does not match the declared geometry is a failure, not a padding opportunity.
5. New formats do not pull an SDK into R2; a system library for the primitive codec (zlib for DEFLATE) is the ceiling, declared at the owning target.

---

## 3. Repo-Local Extensions (managed, per the Conflict Triage Law)

;;INTENTION("R2 CPU Computation & Behavior: pure algorithms, coordinate-agnostic vectors, reactive generic engine, and network protocols.")

`nio/mem.h`, filesystem `io/*` and `nio/relational_memory.h` resolve from Relational Engine, not Vexspoke; network protocols and socket adapters belong in Vexspoke `net/*`. The default build links the native engine `Memory_*` implementation; the Rust byte/string ABI in `relational_engine/memory.h` remains a separate surface, no C/Rust atomic layouts are shared, and imported comparison directories stay outside production closure. R1 keeps engine code and owner storage resident during consumer reloads and excludes active callers before destruction; whole-string replacement is not record-schema migration, and Hotcwap reload integration remains unproved.

`net/http.c` implements BSD socket-backed HTTP; `net/websocket.{h,c}` (the RFC 6455 client) implements handshake, frame codec with client masking, the ping/pong/close control path and continuation reassembly over its own BSD socket, routing `wss://` through the TLS seam behind `net/tls.h`. The Apple TLS backend (`net/tls_apple.m`, Network.framework, verifying against the system trust anchors) proves its refusal paths; `net/tls_curl.c` stays a fail-closed stub elsewhere; the client pushes its poll budget into the session (`Tls_setBudget`) and uses `Tls_readable` for readiness; `ws://` is unchanged and polls its socket directly.

Adapter `net/vex_ws_source.{h,c}`: the same four seam signatures a host's transport table binds — no cross-repo include; R3 does not include the client — api-haven keeps the `WsSource` fn-table shape, a host seeing both layers binds it. The legacy frame inbox remains a caller-fed inbox, still not a WebSocket client. The opt-in live smoke (`tests/vexspoke/net/websocket_smoke_test.c`, targeted via `WS_SMOKE_URL='wss://gateway.discord.gg/?v=10&encoding=json'` — Discord's credential-free gateway, unprompted HELLO decodes without auth) proves a completed secure handshake, real server framing and a cheap idle session. Limit: on a TLS connection `poll` cannot yet distinguish a quiet peer from a closed one — a dropped TLS connection reports `WS_MESSAGE_NONE` until a read or write actually fails — a host needing that distinction must add it.

---

## 4. Readiness Cross-Reference (Living Documentation Law)

- Feature readiness matrix: [vexspoke](https://gist.github.com/vex-graph/6943f92acb931b25dad1073c46da6ce7#file-vexspoke-md).
- Open blockers and deferred decisions: [ecosystem blockers Gist](https://gist.github.com/vex-graph/e921fa188eebbd0c68c4e59646109887).

