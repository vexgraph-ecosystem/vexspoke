#include "input/key_map.h"

#include <stdlib.h>
#include <string.h>
#include "nio/mem.h"
#include "oop/type.h"
#include "input/key.h"
#include "input/mouse.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: KeyMap (input/key_map.c)
 * LEVEL: L2 — Behavior (input binding registry)
 * ============================================================================
 * Input binding registry: maps composable int64_t combo IDs to function
 * pointers. Pure behavior — zero UI, zero GPU, zero steady-state allocation.
 *
 * Combo encoding (14 hex digits):
 *   0x [KMODE][FN][CTRL][OPT][CMD][SHIFT][KEY × 8]
 *       bits 52..55   48   44   40   36    32    0..31
 *
 * Modifier nibbles: bit0=on | bit1..3 reserved (layer/lock/hold)
 *
 * STRUCT FIELDS (Mirroring input/key_map.h):
 * ----------------------------------------------------------------------------
 *   KeyBinding *bindings;       flat array of registered combos (arena-allocated)
 *   uint32_t    count;          active binding count
 *   uint32_t    capacity;       allocated slots (doubling growth on bind)
 *   void       *arena;          opaque MemoryArena handle for allocations
 *   bool        multiTapEnabled;  false = rhythm mode (instant singles, no doubles/triples)
 *   uint64_t    longPressNanos;   LONG_PRESS hold threshold (0 = disabled on this map)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - KeyMap_create(arena)                            : KeyMap_create()
 *
 * Core Functions:
 *   - KeyMap_destroy(map)                             : free bindings + map
 *   - KeyMap_buildCombo(keyCode, gestureType)         : assemble combo from
 *                                                       current modifier state
 *   - KeyMap_resolve(map, outCombo)                   : per-frame gesture
 *                                                       resolution, exact match;
 *                                                       stale taps offered but
 *                                                       declined on modifiers
 *                                                       are expired (never
 *                                                       re-matched later)
 *
 * Setters:
 *   - KeyMap_bind(map, combo, fn, userdata)           : register a binding
 *   - KeyMap_unbind(map, combo, fn)                   : remove a binding
 *   - KeyMap_setMultiTapEnabled(map, enabled)         : rhythm-mode switch
 *   - KeyMap_setLongPressNanos(map, nanos)            : per-map hold threshold
 *
 * Getters:
 *   - KeyMap_match(map, liveCombo)                    : exact-match lookup
 *   - KeyMap_count(map)                               : active binding count
 *   - KeyMap_isEmpty(map)                             : true if count == 0
 *   - KeyMap_isMultiTapEnabled(map)                   : rhythm-mode readback
 *   - KeyMap_getLongPressNanos(map)                   : hold threshold readback
 * ============================================================================
 */

// ── Gesture resolution (KeyMap_resolve) ───────────────────
// Multi-tap recognition is windowed (settle-based): taps are OFFERED while the
// per-key pending window (tapWindowNanos, 250ms on the platform drivers) is
// still open and settle into SINGLE/DOUBLE/TRIPLE once it closes. Resolve
// therefore fires nothing while a gesture is pending; at settlement the most
// specific KMODE binding aware of the settled count fires exactly once (tap
// counters are consumed on hit). Modifier-state changes between presses break
// the sequence (different mods => fresh sequence).
//
// Rhythm-game mode (KeyMap_setMultiTapEnabled(false)): every press-release is
// a single tap resolved immediately — zero window latency; DOUBLE/TRIPLE
// bindings never resolve on this map.
//
// LONG_PRESS fires once per hold (one-shot latch in the driver slot, cleared
// on release). Its hit also consumes the tap count, so releasing the same
// press never ALSO fires a TAP. Threshold is per-map longPressNanos
// (KEYMAP_LONG_PRESS_NANOS by default; 0 disables long-press).
//
// Mouse buttons occupy codes 0..7 (MOUSE_LEFT..MOUSE_BUTTON_8); keyboard
// codes start at 32 (KEY_SPACE), so code < 32 always means a mouse button.
#define KMAP_MOUSE_ID_MAX 32


// ── Initial capacity for the binding array ────────────────
#define KEYMAP_INITIAL_CAPACITY 16

// ── Helpers ───────────────────────────────────────────────

static int64_t modifierState(void)
{
    int64_t mods = 0;
    if (Key_isDown(KEY_LEFT_SHIFT) || Key_isDown(KEY_RIGHT_SHIFT))
        mods |= KMOD_SHIFT;
    if (Key_isDown(KEY_LEFT_SUPER) || Key_isDown(KEY_RIGHT_SUPER))
        mods |= KMOD_CMD;
    if (Key_isDown(KEY_LEFT_ALT) || Key_isDown(KEY_RIGHT_ALT))
        mods |= KMOD_OPT;
    if (Key_isDown(KEY_LEFT_CONTROL) || Key_isDown(KEY_RIGHT_CONTROL))
        mods |= KMOD_CTRL;
    if (Key_isDown(KEY_MAC_FN))
        mods |= KMOD_FN;
    return mods;
}

// Gesture type from tap count (reads vexspoke's multi-tap state).
static int64_t gestureFromTaps(int taps)
{
    if (taps >= 3)
        return KMODE_TRIPLE_TAP;
    if (taps == 2)
        return KMODE_DOUBLE_TAP;
    return KMODE_TAP;
}

// Gesture type from mouse button tap count.
static int64_t mouseGestureFromTaps(int taps)
{
    if (taps >= 3)
        return KMODE_TRIPLE_TAP;
    if (taps == 2)
        return KMODE_DOUBLE_TAP;
    return KMODE_TAP;
}

// Gesture type from hold duration (nanoseconds).
static int64_t gestureFromHoldDuration(uint64_t holdNanos, uint64_t longPressThresholdNanos)
{
    if (longPressThresholdNanos > 0 && holdNanos >= longPressThresholdNanos)
        return KMODE_LONG_PRESS;
    return KMODE_TAP;
}

// True when live Key/Mouse state currently shows the combo's gesture.
// Multi-tap modes read the SETTLED phase (Key_tapPhase / Mouse_tapPhase):
// a pending (window still open) sequence never matches, so nothing fires
// until the count settles. Rhythm mode (multiTapEnabled == false) reads the
// raw tap counter and requires the key/button released — every press-release
// is an instant single. LONG_PRESS reads the hold clock against the map's
// longPressNanos and refuses while the one-shot latch is set (a given press
// fires at most once). DRAG/SCROLL/ZOOM are event-stream gestures: they never
// match under per-frame polling (;;DRAFT — event-driven resolver comes later).
static bool gestureMatches(const KeyMap *map, int64_t combo)
{
    bool multiTap = (map == nullptr) || (*map).multiTapEnabled;
    uint64_t longPressNanos = (map == nullptr) ? KEYMAP_LONG_PRESS_NANOS : (*map).longPressNanos;
    int64_t code = (combo & KEY_CODE_MASK);
    int64_t kmode = (combo & KMODE_MASK);

    if (code < KMAP_MOUSE_ID_MAX) {
        int button = (int) code;
        if (kmode == KMODE_LONG_PRESS) {
            if (longPressNanos == 0)
                return false;
            return Mouse_isDown(button)
                && Mouse_currentHoldDurationNanos(button) >= longPressNanos
                && !Mouse_isLongPressFired(button);
        }
        if (!multiTap) { // rhythm mode: instant singles only
            if (kmode != KMODE_TAP)
                return false;
            return Mouse_taps(button) > 0 && !Mouse_isDown(button);
        }
        MouseTapPhase phase = Mouse_tapPhase(button);
        if (phase == MOUSE_TAP_PENDING)
            return false; // window still open — nothing settles yet
        if (kmode == KMODE_TAP)
            return phase == MOUSE_TAP_SINGLE && !Mouse_isDown(button);
        if (kmode == KMODE_DOUBLE_TAP)
            return phase == MOUSE_TAP_DOUBLE;
        if (kmode == KMODE_TRIPLE_TAP)
            return phase == MOUSE_TAP_TRIPLE;
        return false;
    }

    int key = (int) code;
    if (kmode == KMODE_LONG_PRESS) {
        if (longPressNanos == 0)
            return false;
        return Key_isDown(key)
            && Key_currentHoldDurationNanos(key) >= longPressNanos
            && !Key_isLongPressFired(key);
    }
    if (!multiTap) { // rhythm mode: instant singles only
        if (kmode != KMODE_TAP)
            return false;
        return Key_taps(key) > 0 && !Key_isDown(key);
    }
    KeyTapPhase phase = Key_tapPhase(key);
    if (phase == KEY_TAP_PENDING)
        return false; // window still open — nothing settles yet
    if (kmode == KMODE_TAP)
        return phase == KEY_TAP_SINGLE && !Key_isDown(key);
    if (kmode == KMODE_DOUBLE_TAP)
        return phase == KEY_TAP_DOUBLE;
    if (kmode == KMODE_TRIPLE_TAP)
        return phase == KEY_TAP_TRIPLE;
    return false;
}

// Consume the gesture source after a hit so it cannot re-fire next frame.
// A LONG_PRESS hit also latches the driver slot: while the same press is
// still held nothing re-fires, and the tap count is gone so the release of
// the same press never fires a TAP either.
static void consumeGesture(int64_t combo)
{
    int64_t code = (combo & KEY_CODE_MASK);
    int64_t kmode = (combo & KMODE_MASK);
    if (code < KMAP_MOUSE_ID_MAX) {
        int button = (int) code;
        if (kmode == KMODE_LONG_PRESS)
            Mouse_setLongPressFired(button, true);
        Mouse_resetTaps(button);
    } else {
        int key = (int) code;
        if (kmode == KMODE_LONG_PRESS)
            Key_setLongPressFired(key, true);
        Key_resetTaps(key);
    }
}


// ═══════════════════════════════════════════════════════════
// CONSTRUCTORS
// ═══════════════════════════════════════════════════════════

KeyMap *KeyMap_create(void *arena)
{
    MemoryArena *a = (MemoryArena*) arena;
    if (a == nullptr)
        return nullptr;

    KeyMap *map = (KeyMap*) MemoryArena_alloc(a, TYPE_KEY_MAP_SINGLETON, sizeof(KeyMap));
    if (map == nullptr)
        return nullptr;

    KeyBinding *bindings = (KeyBinding*) MemoryArena_alloc(
        a, TYPE_KEY_MAP_ARRAY, sizeof(KeyBinding) * KEYMAP_INITIAL_CAPACITY
    );
    if (bindings == nullptr)
        return nullptr;

    (*map).bindings  = bindings;
    (*map).count     = 0;
    (*map).capacity  = KEYMAP_INITIAL_CAPACITY;
    (*map).arena     = arena;
    (*map).multiTapEnabled = true; // settle-based multi-tap on by default
    (*map).longPressNanos  = KEYMAP_LONG_PRESS_NANOS;

    return map;
}


// ═══════════════════════════════════════════════════════════
// CORE FUNCTIONS
// ═══════════════════════════════════════════════════════════

void KeyMap_destroy(KeyMap *map)
{
    if (map == nullptr)
        return;
    // Arena owns the memory — bindings and map are freed when
    // MemoryArena_freeAll runs (Teardown Order Law). We NULL the
    // count so stale pointers don't match.
    (*map).count = 0;
    (*map).bindings = nullptr;
}

int64_t KeyMap_buildCombo(int64_t keyCode, int64_t gestureType)
{
    return gestureType | keyCode | modifierState();
}

int64_t KeyMap_buildComboForKey(int key, int64_t gestureType)
{
    int64_t taps = Key_taps(key);
    int64_t gesture = gestureType;
    if (gesture == KMODE_TAP && taps > 1)
        gesture = gestureFromTaps((int) taps);
    return gesture | (int64_t) key | modifierState();
}

int64_t KeyMap_buildComboForMouse(int button, int64_t gestureType)
{
    int taps = Mouse_taps(button);
    int64_t gesture = gestureType;
    if (gesture == KMODE_TAP && taps > 1)
        gesture = mouseGestureFromTaps(taps);
    // Mouse buttons use their enum index (MOUSE_LEFT=0, etc.)
    // as the low bits of the key code.
    return gesture | (int64_t) button | modifierState();
}

int64_t KeyMap_buildComboForHold(int key, uint64_t longPressThresholdNanos)
{
    uint64_t hold = Key_currentHoldDurationNanos(key);
    int64_t gesture = gestureFromHoldDuration(hold, longPressThresholdNanos);
    return gesture | (int64_t) key | modifierState();
}

bool KeyMap_resolve(const KeyMap *map, int64_t *outCombo)
{
    if (map == nullptr)
        return false;

    int64_t liveMods = modifierState() & KMOD_ALL_MASK;
    const KeyBinding *best = nullptr;
    int64_t bestKm = -1;

    for (uint32_t i = 0; i < (*map).count; i++) {
        const KeyBinding *b = &(*map).bindings[i];
        if ((*b).fn == nullptr)
            continue;

        int64_t bm = (*b).combo & KMODE_MASK;
        if (best != nullptr && bm <= bestKm)
            continue; // strictly-less specific, or earlier equal already chosen

        if (((*b).combo & KMOD_ALL_MASK) != liveMods)
            continue; // exact modifier equality

        if (!gestureMatches(map, (*b).combo))
            continue;

        best = b;
        bestKm = bm;
    }

    // Second pass (no fire): expire stale taps that were "offered but
    // declined on modifiers." A binding whose gesture is live (SETTLED) for
    // its code but which was rejected because modifiers differ has consumed
    // its implicit offer — consuming the tap now keeps a bare-Q tap from
    // lingering until Cmd+A arrives later. Pending (window still open)
    // sequences are never consumed here — they may still upgrade. Only the
    // losing modifier-space is expired; gesture-kind upgrades (a settled
    // DOUBLE against a TAP binding) are preserved by phase semantics.
    for (uint32_t i = 0; i < (*map).count; i++) {
        const KeyBinding *b = &(*map).bindings[i];
        if ((*b).fn == nullptr)
            continue;
        if (!gestureMatches(map, (*b).combo))
            continue;
        if (((*b).combo & KMOD_ALL_MASK) == liveMods)
            continue; // would have matched — handled above or already the winner
        consumeGesture((*b).combo);
    }

    if (best == nullptr)
        return false;

    // Consume BEFORE invoking so a re-entrant render cannot re-fire.
    consumeGesture((*best).combo);

    if (outCombo != nullptr)
        *outCombo = (*best).combo;
    (*best).fn((*best).userdata, (*best).combo);
    return true;
}


// ═══════════════════════════════════════════════════════════
// SETTERS
// ═══════════════════════════════════════════════════════════

bool KeyMap_bind(KeyMap *map, int64_t combo, KeyBindingFn fn, void *userdata)
{
    if (map == nullptr || fn == nullptr)
        return false;

    // Check if this combo is already bound — replace in-place.
    for (uint32_t i = 0; i < (*map).count; i++) {
        if ((*map).bindings[i].combo == combo && (*map).bindings[i].fn == fn) {
            (*map).bindings[i].userdata = userdata;
            return true;
        }
    }

    // Grow if full (doubling).
    if ((*map).count >= (*map).capacity) {
        uint32_t newCap = (*map).capacity * 2;
        MemoryArena *a = (MemoryArena*) (*map).arena;
        KeyBinding *newBindings = (KeyBinding*) MemoryArena_alloc(
            a, TYPE_KEY_MAP_ARRAY, sizeof(KeyBinding) * newCap
        );
        if (newBindings == nullptr)
            return false;
        memcpy(newBindings, (*map).bindings, sizeof(KeyBinding) * (*map).count);
        (*map).bindings  = newBindings;
        (*map).capacity  = newCap;
    }

    // Append.
    uint32_t slot = (*map).count;
    (*map).bindings[slot].combo    = combo;
    (*map).bindings[slot].fn       = fn;
    (*map).bindings[slot].userdata = userdata;
    (*map).count = slot + 1;

    return true;
}

bool KeyMap_unbind(KeyMap *map, int64_t combo, KeyBindingFn fn)
{
    if (map == nullptr || fn == nullptr)
        return false;

    for (uint32_t i = 0; i < (*map).count; i++) {
        if ((*map).bindings[i].combo == combo && (*map).bindings[i].fn == fn) {
            // Swap-remove: move last entry into this slot (O(1)).
            uint32_t last = (*map).count - 1;
            if (i != last)
                (*map).bindings[i] = (*map).bindings[last];
            (*map).count = last;
            return true;
        }
    }
    return false;
}

void KeyMap_setMultiTapEnabled(KeyMap *map, bool enabled)
{
    if (map == nullptr)
        return;
    (*map).multiTapEnabled = enabled;
}

void KeyMap_setLongPressNanos(KeyMap *map, uint64_t nanos)
{
    if (map == nullptr)
        return;
    (*map).longPressNanos = nanos;
}


// ═══════════════════════════════════════════════════════════
// GETTERS
// ═══════════════════════════════════════════════════════════

const KeyBinding *KeyMap_match(const KeyMap *map, int64_t liveCombo)
{
    if (map == nullptr)
        return nullptr;

    for (uint32_t i = 0; i < (*map).count; i++) {
        if ((*map).bindings[i].combo == liveCombo)
            return &(*map).bindings[i];
    }
    return nullptr;
}

uint32_t KeyMap_count(const KeyMap *map)
{
    if (map == nullptr)
        return 0;
    return (*map).count;
}

bool KeyMap_isEmpty(const KeyMap *map)
{
    if (map == nullptr)
        return true;
    return (*map).count == 0;
}

bool KeyMap_isMultiTapEnabled(const KeyMap *map)
{
    if (map == nullptr)
        return false;
    return (*map).multiTapEnabled;
}

uint64_t KeyMap_getLongPressNanos(const KeyMap *map)
{
    if (map == nullptr)
        return 0;
    return (*map).longPressNanos;
}
