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
 *   KeyBinding *bindings;     flat array of registered combos (arena-allocated)
 *   uint32_t    count;        active binding count
 *   uint32_t    capacity;     allocated slots (doubling growth on bind)
 *   void       *arena;        opaque MemoryArena handle for allocations
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
 *
 * Setters:
 *   - KeyMap_bind(map, combo, fn, userdata)           : register a binding
 *   - KeyMap_unbind(map, combo, fn)                   : remove a binding
 *
 * Getters:
 *   - KeyMap_match(map, liveCombo)                    : exact-match lookup
 *   - KeyMap_count(map)                               : active binding count
 *   - KeyMap_isEmpty(map)                             : true if count == 0
 * ============================================================================
 */


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
