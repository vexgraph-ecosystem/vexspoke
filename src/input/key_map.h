#ifndef INPUT_KEY_MAP_H
#define INPUT_KEY_MAP_H

#include <stdint.h>
#include <stdbool.h>

#include "annotation/draft.h"
#include "annotation/intention.h"

// input/key_map.h — input binding registry (combo → fn pointer).
//
// Maps composable int64_t combo IDs to function pointers. Pure behavior
// (R2 vexspoke) — zero UI, zero GPU. Any consumer (Frame, Application,
// Console) can own a KeyMap.
//
// Combo encoding (14 hex digits):
//   0x [KMODE][FN][CTRL][OPT][CMD][SHIFT][KEY × 8]
//       bits 52..55   48   44   40   36    32    0..31
//
// Modifier nibbles: bit0=on | bit1..3 reserved (layer/lock/hold)
// reserved = leverage: encoding never changes, matcher reads deeper later.
//
// Key mode nibble:
//   0 = TAP, 1 = DOUBLE_TAP, 2 = TRIPLE_TAP, 3 = LONG_PRESS
//   4 = DRAG, 5 = SCROLL, 6 = ZOOM, 7..F = reserved
//
// Key codes reuse vexspoke key.h / mouse.h values (KEY_A..KEY_Z, MOUSE_LEFT..).

// ── Combo constants: key mode (nibble at bits 52..55) ─────
#define KMODE_TAP          (0LL << 52)
#define KMODE_DOUBLE_TAP   (1LL << 52)
#define KMODE_TRIPLE_TAP   (2LL << 52)
#define KMODE_LONG_PRESS   (3LL << 52)
#define KMODE_DRAG         (4LL << 52)
#define KMODE_SCROLL       (5LL << 52)
#define KMODE_ZOOM         (6LL << 52)
// 0x7..0xF reserved

// ── Combo constants: modifier nibbles (each 4 bits) ───────
// bit0 = on, bit1..3 = reserved (layer/lock/hold tiers)
// KMOD_* prefixed to avoid colliding with key.h key-code constants
// (KEY_FN is key code 349; these are modifier flags, not key codes).
#define KMOD_SHIFT         (1LL << 32)
#define KMOD_CMD           (1LL << 36)   // Mac CMD / Linux SUPER
#define KMOD_OPT           (1LL << 40)   // Mac OPTION / Windows ALT
#define KMOD_CTRL          (1LL << 44)
#define KMOD_FN            (1LL << 48)

// ── Masks ──────────────────────────────────────────────────
// KMOD_ALL_MASK covers all 5 modifier nibbles (bits 32..51).
// KEY_CODE_MASK / KMODE_MASK isolate their fields for matchMask-style work.
#define KMOD_ALL_MASK      0x000FFFFF00000000LL   // SHIFT|CMD|OPT|CTRL|FN nibbles
#define KEY_CODE_MASK      0x00000000FFFFFFFFLL   // key code bits only (bits 0..31)
#define KMODE_MASK         0x00F0000000000000LL   // gesture mode nibble (bits 52..55)

// Per-frame gesture resolution reads hold state against this threshold.
#define KEYMAP_LONG_PRESS_NANOS (400ull * 1000 * 1000) // 400 ms hold → LONG_PRESS

// ── Types ─────────────────────────────────────────────────

typedef void (*KeyBindingFn)(void *userdata, int64_t combo);

typedef struct KeyBinding {
    int64_t       combo;       // the encoded combo
    KeyBindingFn  fn;          // callback
    void         *userdata;    // opaque context
} KeyBinding;

typedef struct KeyMap {
    KeyBinding *bindings;      // flat array of registered combos
    uint32_t    count;         // active binding count
    uint32_t    capacity;      // allocated slots (doubling growth)
    void       *arena;         // opaque MemoryArena handle
    bool        multiTapEnabled; // false = rhythm mode: every press an instant single
    uint64_t    longPressNanos;  // LONG_PRESS hold threshold (0 = disabled on this map)
} KeyMap;

// ── Lifecycle ─────────────────────────────────────────────

;;DRAFT
;;INTENTION("exact-match only by design (2026-09-16 session); wildcard "
           "matching is the known next step — KeyMap_matchMask(map, combo, "
           "mask) planned for any-modifier/any-key binds; do not harden the "
           "public surface until a real Frame consumer proves it")

KeyMap *KeyMap_create(void *arena);
void    KeyMap_destroy(KeyMap *map);

// ── Binding ───────────────────────────────────────────────
// Exact match: liveCombo must equal the registered combo exactly.
// Wildcard matching (KeyMap_matchMask) deferred — see DRAFT annotation above.

bool KeyMap_bind(KeyMap *map, int64_t combo, KeyBindingFn fn, void *userdata);
bool KeyMap_unbind(KeyMap *map, int64_t combo, KeyBindingFn fn);

// ── Matching ──────────────────────────────────────────────
// Scan bindings for an exact combo match. Returns the binding or NULL.
// O(n) where n = binding count — fast for <64 bindings.

const KeyBinding *KeyMap_match(const KeyMap *map, int64_t liveCombo);

// ── Resolution (per-frame) ────────────────────────────────
// Resolve live Key/Mouse gesture state against the binding table.
// Exact-match semantics: a binding fires when its gesture
// (TAP/DOUBLE_TAP/TRIPLE_TAP/LONG_PRESS) is present in live state for its
// code, and its modifier bits EQUAL the current modifier state (no
// unbound-modifier tolerance — wildcard tolerance is KeyMap_matchMask,
// deferred). When several bindings match, the most specific gesture wins
// (higher KMODE); ties resolve in bind order. At most one binding fires per
// call: its fn(userdata, combo) runs and the source tap counter is consumed
// (Key_resetTaps / Mouse_resetTaps) so the hit cannot re-fire on the next
// frame.
//
// Windowed settlement (multi-tap on, the default): taps are OFFERED while the
// per-key pending window (tapWindowNanos, 250ms on the platform drivers) is
// still open — nothing fires until the window closes after the LAST press and
// the count settles into SINGLE/DOUBLE/TRIPLE. TAP bindings additionally
// require the key/button released (release owns the single); DOUBLE/TRIPLE
// fire at settlement. A sequence that settled but was declined on modifiers
// is expired (never re-matched later). Modifier-state changes between presses
// break the sequence.
//
// Multi-tap off (rhythm game mode, KeyMap_setMultiTapEnabled(false)): every
// press-release is a single tap resolved immediately — zero window latency;
// DOUBLE/TRIPLE bindings never resolve.
//
// LONG_PRESS fires once per press (one-shot latch in the driver slot, cleared
// on release), consuming the tap count so the release of the same press never
// also fires a TAP. DRAG/SCROLL/ZOOM combos can be bound but never resolve
// from per-frame polling — they are event-stream gestures reserved for the
// future event-driven resolver (;;DRAFT). Long-press uses the map's
// longPressNanos threshold (KEYMAP_LONG_PRESS_NANOS by default; 0 disables).

bool KeyMap_resolve(const KeyMap *map, int64_t *outCombo);

// ── Recognition config ────────────────────────────────────
// multiTapEnabled (default true): settle-based multi-tap recognition. Turning
// it off switches to rhythm-game mode — instant singles, no window latency,
// no double/triple resolution (bindings for them never fire).
bool KeyMap_isMultiTapEnabled(const KeyMap *map);
void KeyMap_setMultiTapEnabled(KeyMap *map, bool enabled);

// Hold threshold for LONG_PRESS resolution (default KEYMAP_LONG_PRESS_NANOS;
// 0 disables long-press bindings on this map).
uint64_t KeyMap_getLongPressNanos(const KeyMap *map);
void     KeyMap_setLongPressNanos(KeyMap *map, uint64_t nanos);

// ── Combo builder ─────────────────────────────────────────
// Reads current modifier state from vexspoke's Key_isDown / Key_taps,
// assembles the full int64_t combo from a key code + gesture type.
// The caller invokes this on each input event to build the live combo,
// then passes it to KeyMap_match.

int64_t KeyMap_buildCombo(int64_t keyCode, int64_t gestureType);

// Convenience builders that fold in live tap/hold state:
// - Key: gesture comes from Key_taps(code) when gestureType is TAP.
// - Mouse: gesture comes from Mouse_taps(button) when gestureType is TAP.
// - Hold: gesture is LONG_PRESS when the current hold exceeds the threshold
//   (nanoseconds; 0 disables long-press detection).
int64_t KeyMap_buildComboForKey(int key, int64_t gestureType);
int64_t KeyMap_buildComboForMouse(int button, int64_t gestureType);
int64_t KeyMap_buildComboForHold(int key, uint64_t longPressThresholdNanos);

// ── Query ─────────────────────────────────────────────────

uint32_t KeyMap_count(const KeyMap *map);
bool     KeyMap_isEmpty(const KeyMap *map);

#endif
