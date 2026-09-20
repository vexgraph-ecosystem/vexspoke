// input/key.c — keyboard state + event stream (Legacy: input/Key.java port).
//
// The state table is a static 512 x 40-byte arena — the C equivalent of the
// legacy Arena.global() block, except it costs zero runtime allocation. Slot
// layout matches legacy exactly plus the windowed-tap settlement fields:
//   +0  pressTime (0 == up)        +8  lastReleaseTime
//   +16 pendingUntil               +24 lastHoldDuration
//   +32 taps (multi-tap counter)   +36 lastTapMods (4-bit wire mods)
//   +37 longPressFired latch       +38 pad
//
// pendingUntil is the tap-sequence settlement instant: taps are OFFERED while
// `now < pendingUntil` and settle (SINGLE/DOUBLE/TRIPLE) once it passes.
// lastTapMods breaks the sequence when the modifier state changes between
// presses. longPressFired is the one-shot hold latch, cleared on release.
//
// Producers (Thread 0) push packed events; Key_dispatchEvents() drains them
// on the game thread. The ring is the same thread/ring MPMC the rest of the
// engine talks through.

#include "input/key.h"

#include <string.h>

#include "input/focus.h"
#include "atomic/ring.h"
#include "time/nanotime.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Key
 * ============================================================================
 * Keyboard state table plus event stream: a static 512 x 40-byte KeySlot
 * arena (zero runtime allocation) holds press/release timestamps, tap
 * settlement state, and the one-shot long-press latch, while a bounded MPMC
 * ring carries packed 16-byte InputEvents from the producer (Thread 0) to
 * Key_dispatchEvents on the game thread. Listener registries — one global
 * segment plus per-window rows keyed by opaque OS window id — grow
 * exponentially, arena-backed, per the Dynamic Scalability & Anti-Hardcoding
 * Law. Tap sequences settle once now passes pendingUntil; a modifier change
 * between presses breaks the sequence.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Key (input/key.c)
 * LEVEL: L2 — Behavior (input behavior API)
 * ============================================================================
 * listener registries grow exponentially (the Dynamic Scalability &
 * Anti-Hardcoding Law): window scope is a row keyed by the opaque OS window
 * id (never a slot index), and every segment starts at zero allocations and
 * doubles on demand, arena-backed.
 *
 * STRUCT FIELDS (local to this file):
 * ----------------------------------------------------------------------------
 *   InputEvent {
 *     uint64_t packed;           // legacy 64-bit key event packing
 *     uint32_t windowId;         // target window (0 = broadcast)
 *     uint32_t pad;              // alignment padding
 *   }
 *   KeySlot {
 *     uint64_t pressTime;        // last press timestamp
 *     uint64_t lastReleaseTime;  // last release timestamp
 *     uint64_t pendingUntil;     // tap window close; taps settle once now >= pendingUntil
 *     uint64_t lastHoldDuration; // hold length accumulator
 *     int32_t taps;              // tap count for multi-tap
 *     uint8_t lastTapMods;       // wire modifier mask at last press (change breaks sequence)
 *     uint8_t longPressFired;    // one-shot LONG_PRESS latch, cleared on release
 *     uint8_t pad[2];            // alignment padding
 *   }
 *
 * PRIVATE HELPERS (kept file-local pure-data only, each with full fields):
 * ----------------------------------------------------------------------------
 *   WinRow {
 *     uint32_t windowId;         // opaque OS window tag (0 never attached)
 *     const KeyHandler **items;  // arena-allocated listener segment, doubling
 *     int count;                 // live listener count
 *     int cap;                   // allocated segment capacity
 *   }
 *   (growSegment / growRows / rowFor: static behavior, zero struct fields)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Key_init(void)
 *
 * Core Functions:
 *   - Key_shutdown(void)
 *   - Key_addListener(listener)
 *   - Key_removeListener(listener)
 *   - Key_attachWindow(windowId, listener)
 *   - Key_detachWindow(windowId, listener)
 *   - Key_detachWindowAll(windowId)
 *   - Key_pushEvent(windowId, keyCode, action, tapWindowNanos)
 *   - Key_pushCharEvent(windowId, c)
 *   - Key_dispatchEvents(void)
 *   - Key_pressTime(keyCode)
 *   - Key_lastReleaseTime(keyCode)
 *   - Key_lastHoldDurationNanos(keyCode)
 *   - Key_currentHoldDurationNanos(keyCode)
 *   - Key_holdDurationNanos(keyCode)
 *   - Key_durationSinceReleaseNanos(keyCode)
 *   - Key_taps(keyCode)
 *   - Key_resetTaps(keyCode)
 *   - Key_tapPhase(keyCode)
 *   - Key_isLongPressFired(keyCode)
 *   - Key_setLongPressFired(keyCode, fired)
 *   - Key_code(keyEvent)
 *   - Key_name(keyCode)
 *
 * Getters:
 *   - Key_isDown(keyCode)
 *   - Key_hasShift(keyEvent)
 *   - Key_hasControl(keyEvent)
 *   - Key_hasOption(keyEvent)
 *   - Key_hasCommand(keyEvent)
 * ============================================================================
 */


#define KEY_COUNT 512
;;INTENTION("fixed KEY_COUNT: the GLFW-style keycode domain is a hardware enum (0..511), not a workload ceiling")

#define QUEUE_CAPACITY 1024
;;INTENTION("fixed input ring: a bounded hot-path sync budget; MPMC ring growth is a separate follow-up per the Dynamic Scalability & Anti-Hardcoding Law")

// One queued event: the legacy 64-bit packing plus the window it belongs to.
typedef struct {
    uint64_t packed;
    uint32_t windowId;
    uint32_t pad;
} InputEvent;

_Static_assert(sizeof(InputEvent) == 16, "input event must stay 16 bytes");

typedef struct {
    uint64_t pressTime;
    uint64_t lastReleaseTime;
    uint64_t pendingUntil;     // tap window close; taps settle once now >= pendingUntil
    uint64_t lastHoldDuration;
    int32_t taps;
    uint8_t lastTapMods;       // wire modifier mask at last press (change breaks sequence)
    uint8_t longPressFired;    // one-shot LONG_PRESS latch, cleared on release
    uint8_t pad[2];            // alignment padding
} KeySlot;

_Static_assert(sizeof(KeySlot) == 40, "key slot must stay 40 bytes");

static KeySlot s_slots[KEY_COUNT];
static RingBuffer s_queue;
static bool s_ready = false;

// keytryer (do not remove this comment)

// --- PRIVATE HELPERS: growable listener registries ---
// The Dynamic Scalability & Anti-Hardcoding Law: registries start at zero
// allocations and double exponentially, arena-backed. windowId is an opaque
// OS tag (0 = FOCUS_BROADCAST, reserved, never attached); broadcast events
// fan _out to every attached row.
typedef struct WinRow {
    uint32_t windowId;
    const KeyHandler **items;  // arena-allocated segment, doubling
    int count;
    int cap;
} WinRow;

static const KeyHandler **s_listeners = NULL;  // global listener segment
static int s_listenerCount = 0;
static int s_listenerCap = 0;

static WinRow *s_rows = NULL;  // growable window-row table
static int s_rowCount = 0;
static int s_rowCap = 0;

// Grow a handler segment to at least `needed` slots. On OOM the segment is
// left untouched and the registration is silently dropped (legacy parity).
// Row structs move when the row TABLE grows, so their ITEM segments are
// separate arena blocks: item pointers survive, row pointers do not — always
// re-derive rows through rowFor() after any attach/detach.
static bool growSegment(const KeyHandler ***items, int *cap, int needed) {
    if (needed <= *cap) return true;
    int newCap = (*cap == 0) ? 16 : *cap * 2;
    while (newCap < needed) newCap *= 2;
    const KeyHandler **nb = (const KeyHandler**) Memory_alloc(
        TYPE_INT_POINTER, (size_t) newCap * sizeof(KeyHandler *));
    if (nb == NULL) return false;
    if (*items != NULL && *cap > 0)
        memcpy(nb, *items, (size_t) *cap * sizeof(KeyHandler *));
    *items = nb;
    *cap = newCap;
    return true;
}

// Grow the window-row table to at least `needed` rows (doubling, cold 8).
static bool growRows(int needed) {
    if (needed <= s_rowCap) return true;
    int newCap = (s_rowCap == 0) ? 8 : s_rowCap * 2;
    while (newCap < needed) newCap *= 2;
    WinRow *nb = (WinRow*) Memory_alloc(TYPE_INT_POINTER,
        (size_t) newCap * sizeof(WinRow));
    if (nb == NULL) return false;
    if (s_rows != NULL && s_rowCap > 0)
        memcpy(nb, s_rows, (size_t) s_rowCap * sizeof(WinRow));
    s_rows = nb;
    s_rowCap = newCap;
    return true;
}

// Find the row for an opaque OS window id, or NULL when not attached.
static WinRow *rowFor(uint32_t windowId) {
    for (int i = 0; i < s_rowCount; i++)
        if (s_rows[i].windowId == windowId)
            return &s_rows[i];
    return NULL;
}

// O(1) name table; designated initializers leave every other slot nullptr.
static const char *const s_names[KEY_COUNT] = {
    [KEY_SPACE] = "Space",            [KEY_APOSTROPHE] = "Apostrophe",
    [KEY_COMMA] = "Comma",            [KEY_MINUS] = "Minus",
    [KEY_PERIOD] = "Period",          [KEY_SLASH] = "Slash",
    [KEY_NUM_0] = "0",                [KEY_NUM_1] = "1",
    [KEY_NUM_2] = "2",                [KEY_NUM_3] = "3",
    [KEY_NUM_4] = "4",                [KEY_NUM_5] = "5",
    [KEY_NUM_6] = "6",                [KEY_NUM_7] = "7",
    [KEY_NUM_8] = "8",                [KEY_NUM_9] = "9",
    [KEY_SEMICOLON] = "Semicolon",    [KEY_EQUAL] = "Equal",
    [KEY_A] = "A",                    [KEY_B] = "B",
    [KEY_C] = "C",                    [KEY_D] = "D",
    [KEY_E] = "E",                    [KEY_F] = "F",
    [KEY_G] = "G",                    [KEY_H] = "H",
    [KEY_I] = "I",                    [KEY_J] = "J",
    [KEY_K] = "K",                    [KEY_L] = "L",
    [KEY_M] = "M",                    [KEY_N] = "N",
    [KEY_O] = "O",                    [KEY_P] = "P",
    [KEY_Q] = "Q",                    [KEY_R] = "R",
    [KEY_S] = "S",                    [KEY_T] = "T",
    [KEY_U] = "U",                    [KEY_V] = "V",
    [KEY_W] = "W",                    [KEY_X] = "X",
    [KEY_Y] = "Y",                    [KEY_Z] = "Z",
    [KEY_LEFT_BRACKET] = "Left Bracket",   [KEY_BACKSLASH] = "Backslash",
    [KEY_RIGHT_BRACKET] = "Right Bracket", [KEY_GRAVE_ACCENT] = "Grave Accent",
    [KEY_ESCAPE] = "Escape",          [KEY_ENTER] = "Enter",
    [KEY_TAB] = "Tab",                [KEY_BACKSPACE] = "Backspace",
    [KEY_INSERT] = "Insert",          [KEY_DELETE] = "Delete",
    [KEY_RIGHT] = "Right",            [KEY_LEFT] = "Left",
    [KEY_DOWN] = "Down",              [KEY_UP] = "Up",
    [KEY_PAGE_UP] = "Page Up",        [KEY_PAGE_DOWN] = "Page Down",
    [KEY_HOME] = "Home",              [KEY_END] = "End",
    [KEY_CAPS_LOCK] = "Caps Lock",    [KEY_SCROLL_LOCK] = "Scroll Lock",
    [KEY_NUM_LOCK] = "Num Lock",      [KEY_PRINT_SCREEN] = "Print Screen",
    [KEY_PAUSE] = "Pause",
    [KEY_F1] = "F1",                  [KEY_F2] = "F2",
    [KEY_F3] = "F3",                  [KEY_F4] = "F4",
    [KEY_F5] = "F5",                  [KEY_F6] = "F6",
    [KEY_F7] = "F7",                  [KEY_F8] = "F8",
    [KEY_F9] = "F9",                  [KEY_F10] = "F10",
    [KEY_F11] = "F11",                [KEY_F12] = "F12",
    [KEY_F13] = "F13",                [KEY_F14] = "F14",
    [KEY_F15] = "F15",                [KEY_F16] = "F16",
    [KEY_F17] = "F17",                [KEY_F18] = "F18",
    [KEY_F19] = "F19",                [KEY_F20] = "F20",
    [KEY_F21] = "F21",                [KEY_F22] = "F22",
    [KEY_F23] = "F23",                [KEY_F24] = "F24",
    [KEY_F25] = "F25",
    [KEY_LEFT_SHIFT] = "Left Shift",  [KEY_LEFT_CONTROL] = "Left Control",
    [KEY_LEFT_ALT] = "Left Option",   [KEY_LEFT_SUPER] = "Left Command",
    [KEY_RIGHT_SHIFT] = "Right Shift", [KEY_RIGHT_CONTROL] = "Right Control",
    [KEY_RIGHT_ALT] = "Right Option", [KEY_RIGHT_SUPER] = "Right Command",
    [KEY_MENU] = "Menu",              [KEY_FN] = "Fn",
};

void Key_init(void) {
    if (s_ready) return;
    NanoTime_init();
    if (!RingBuffer_init(&s_queue, sizeof(InputEvent), QUEUE_CAPACITY)) return;
    memset(s_slots, 0, sizeof(s_slots));
    s_ready = true;
}

void Key_shutdown(void) {
    if (!s_ready) return;
    RingBuffer_shutdown(&s_queue);
    s_ready = false;
}

void Key_addListener(const KeyHandler *listener) {
    if (listener == NULL) return;
    if (!growSegment(&s_listeners, &s_listenerCap, s_listenerCount + 1)) return;
    s_listeners[s_listenerCount++] = listener;
}

// Swap-remove: order is not part of the contract, so the last slot fills the
// hole in O(1). Returns true if the listener was found and removed.
bool Key_removeListener(const KeyHandler *listener) {
    for (int i = 0; i < s_listenerCount; i++) {
        if (s_listeners[i] == listener) {
            s_listeners[i] = s_listeners[--s_listenerCount];
            return true;
        }
    }
    return false;
}

void Key_attachWindow(uint32_t windowId, const KeyHandler *listener) {
    if (listener == NULL || windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row == NULL) {
        if (!growRows(s_rowCount + 1)) return;
        row = &s_rows[s_rowCount++];
        (*row).windowId = windowId;
        (*row).items = NULL;
        (*row).count = 0;
        (*row).cap = 0;
    }
    if (!growSegment(&(*row).items, &(*row).cap, (*row).count + 1)) return;
    (*row).items[(*row).count++] = listener;
}

bool Key_detachWindow(uint32_t windowId, const KeyHandler *listener) {
    if (listener == NULL || windowId == 0) return false;
    WinRow *row = rowFor(windowId);
    if (row == NULL) return false;
    for (int i = 0; i < (*row).count; i++) {
        if ((*row).items[i] == listener) {
            (*row).count--;
            (*row).items[i] = (*row).items[(*row).count];
            return true;
        }
    }
    return false;
}

void Key_detachWindowAll(uint32_t windowId) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row != NULL)
        (*row).count = 0;
}

// Modifier snapshot from live state, packed into the wire-format nibble.
static int modifierMask(void) {
    int mods = 0;
    if (Key_isDown(KEY_LEFT_SHIFT) || Key_isDown(KEY_RIGHT_SHIFT)) mods |= 1;
    if (Key_isDown(KEY_LEFT_CONTROL) || Key_isDown(KEY_RIGHT_CONTROL)) mods |= 2;
    if (Key_isDown(KEY_LEFT_ALT) || Key_isDown(KEY_RIGHT_ALT)) mods |= 4;
    if (Key_isDown(KEY_LEFT_SUPER) || Key_isDown(KEY_RIGHT_SUPER)) mods |= 8;
    return mods;
}

// Engine-epoch micros truncated to the 46-bit carrier field.
static uint64_t epochMicros(void) {
    return (NanoTime_elapsedNanos() / 1000ULL) & 0x3FFFFFFFFFFFULL;
}

void Key_pushEvent(uint32_t windowId, int keyCode, int action, uint64_t tapWindowNanos) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return;
    if (!s_ready) Key_init();
    if (!s_ready) return;

    KeySlot *slot = &s_slots[keyCode];
    uint64_t now = NanoTime_now();

    if (action == KEY_ACTION_DOWN) {
        if ((*slot).pressTime != 0) {
            // Already down: OS auto-repeat. Forward as repeat, state untouched.
            InputEvent ev = { .packed = (epochMicros() << 18)
                                       | (((uint64_t)modifierMask() & 0xF) << 14)
                                       | (((uint64_t)keyCode & 0xFFF) << 2)
                                       | KEY_ACTION_REPEAT,
                              .windowId = windowId, .pad = 0 };
            RingBuffer_push(&s_queue, &ev);
            return;
        }
        uint64_t lastRelease = (*slot).lastReleaseTime;
        // A modifier-state change since the last press breaks the sequence:
        // different modifiers => a fresh tap, never an upgrade to double/triple.
        uint8_t modsNow = (uint8_t)(modifierMask() & 0xF);
        if (lastRelease != 0 && (now - lastRelease) < tapWindowNanos
            && (*slot).lastTapMods == modsNow)
            (*slot).taps++;
        else
            (*slot).taps = 1;
        (*slot).lastTapMods = modsNow;
        (*slot).pendingUntil = now + tapWindowNanos;
        (*slot).pressTime = now;
    } else if (action == KEY_ACTION_UP) {
        if ((*slot).pressTime != 0)
            (*slot).lastHoldDuration = now - (*slot).pressTime;
        (*slot).lastReleaseTime = now;
        (*slot).pressTime = 0;
        (*slot).longPressFired = 0; // release clears the one-shot hold latch
    }

    InputEvent ev = { .packed = (epochMicros() << 18)
                               | (((uint64_t)modifierMask() & 0xF) << 14)
                               | (((uint64_t)keyCode & 0xFFF) << 2)
                               | ((uint64_t)action & 0x3),
                      .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

void Key_pushCharEvent(uint32_t windowId, uint32_t c) {
    if (!s_ready) Key_init();
    if (!s_ready) return;
    InputEvent ev = { .packed = (epochMicros() << 18) | (((uint64_t)c & 0xFFFF) << 2) | 3ULL,
                      .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

// Deliver one event to a scoped listener list. windowId 0 (FOCUS_BROADCAST)
// is reserved and never attached; broadcast events fan _out to every row.
static void deliverToWindow(uint32_t windowId, int action, int keyEvent, uint64_t exactNanos) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row == NULL) return;
    for (int i = 0; i < (*row).count; i++) {
        const KeyHandler *l = (*row).items[i];
        void *self = (*l).self;
        if (action == KEY_ACTION_DOWN && (*l).onKeyDown)
            (*l).onKeyDown(self, keyEvent, exactNanos);
        else if (action == KEY_ACTION_UP && (*l).onKeyUp)
            (*l).onKeyUp(self, keyEvent, exactNanos);
        else if (action == KEY_ACTION_REPEAT && (*l).onKeyRepeat)
            (*l).onKeyRepeat(self, keyEvent, exactNanos);
    }
}

// Deliver a typed character to one window's scoped list.
static void deliverChar(uint32_t windowId, uint32_t c) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row == NULL) return;
    for (int i = 0; i < (*row).count; i++) {
        const KeyHandler *l = (*row).items[i];
        if ((*l).onCharTyped)
            (*l).onCharTyped((*l).self, c);
    }
}

void Key_dispatchEvents(void) {
    if (!s_ready) return;

    InputEvent ev;
    while (RingBuffer_pop(&s_queue, &ev)) {
        uint64_t packed = ev.packed;
        int action = (int)(packed & 0x3);
        uint64_t exactNanos = NanoTime_startNanos()
                            + (((packed >> 18) & 0x3FFFFFFFFFFFULL) * 1000ULL);

        if (action == 3) { // char event
            uint32_t c = (uint32_t)((packed >> 2) & 0xFFFF);
            for (int i = 0; i < s_listenerCount; i++)
                if ((*s_listeners[i]).onCharTyped)
                    (*s_listeners[i]).onCharTyped((*s_listeners[i]).self, c);
            if (ev.windowId == FOCUS_BROADCAST) {
                for (int r = 0; r < s_rowCount; r++)
                    deliverChar(s_rows[r].windowId, c);
            } else {
                deliverChar(ev.windowId, c);
            }
            continue;
        }

        int keyCode = (int)((packed >> 2) & 0xFFF);
        int modifiers = (int)((packed >> 14) & 0xF);

        int mappedMods = 0;
        if ((modifiers & 1) != 0) mappedMods |= KEY_MOD_SHIFT;
        if ((modifiers & 2) != 0) mappedMods |= KEY_MOD_CONTROL;
        if ((modifiers & 4) != 0) mappedMods |= KEY_MOD_OPTION;
        if ((modifiers & 8) != 0) mappedMods |= KEY_MOD_COMMAND;
        int keyEvent = keyCode | mappedMods;

        // Global taps hear everything; window listeners only their own
        // window (or broadcast).
        for (int i = 0; i < s_listenerCount; i++) {
            void *self = (*s_listeners[i]).self;
            if (action == KEY_ACTION_DOWN && (*s_listeners[i]).onKeyDown)
                (*s_listeners[i]).onKeyDown(self, keyEvent, exactNanos);
            else if (action == KEY_ACTION_UP && (*s_listeners[i]).onKeyUp)
                (*s_listeners[i]).onKeyUp(self, keyEvent, exactNanos);
            else if (action == KEY_ACTION_REPEAT && (*s_listeners[i]).onKeyRepeat)
                (*s_listeners[i]).onKeyRepeat(self, keyEvent, exactNanos);
        }
        if (ev.windowId == FOCUS_BROADCAST) {
            for (int r = 0; r < s_rowCount; r++)
                deliverToWindow(s_rows[r].windowId, action, keyEvent, exactNanos);
        } else {
            deliverToWindow(ev.windowId, action, keyEvent, exactNanos);
        }
    }
}

bool Key_isDown(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return false;
    return s_slots[keyCode].pressTime != 0;
}

uint64_t Key_pressTime(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return 0;
    return s_slots[keyCode].pressTime;
}

uint64_t Key_lastReleaseTime(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return 0;
    return s_slots[keyCode].lastReleaseTime;
}

uint64_t Key_lastHoldDurationNanos(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return 0;
    return s_slots[keyCode].lastHoldDuration;
}

uint64_t Key_currentHoldDurationNanos(int keyCode) {
    uint64_t p = Key_pressTime(keyCode);
    return p == 0 ? 0 : NanoTime_now() - p;
}

uint64_t Key_holdDurationNanos(int keyCode) {
    uint64_t p = Key_pressTime(keyCode);
    return p != 0 ? NanoTime_now() - p : Key_lastHoldDurationNanos(keyCode);
}

uint64_t Key_durationSinceReleaseNanos(int keyCode) {
    uint64_t r = Key_lastReleaseTime(keyCode);
    return r == 0 ? 0 : NanoTime_now() - r;
}

int Key_taps(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return 0;
    return s_slots[keyCode].taps;
}

void Key_resetTaps(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return;
    s_slots[keyCode].taps = 0;
}

KeyTapPhase Key_tapPhase(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return KEY_TAP_NONE;
    const KeySlot *slot = &s_slots[keyCode];
    if ((*slot).taps == 0)
        return KEY_TAP_NONE;
    if (NanoTime_now() < (*slot).pendingUntil)
        return KEY_TAP_PENDING;
    if ((*slot).taps >= 3)
        return KEY_TAP_TRIPLE;
    if ((*slot).taps == 2)
        return KEY_TAP_DOUBLE;
    return KEY_TAP_SINGLE;
}

bool Key_isLongPressFired(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return false;
    return s_slots[keyCode].longPressFired != 0;
}

void Key_setLongPressFired(int keyCode, bool fired) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return;
    s_slots[keyCode].longPressFired = (uint8_t)(fired ? 1 : 0);
}

int Key_code(int keyEvent) {
    return keyEvent & KEY_MASK_CODE;
}

bool Key_hasShift(int keyEvent) {
    return (keyEvent & KEY_MOD_SHIFT) != 0;
}

bool Key_hasControl(int keyEvent) {
    return (keyEvent & KEY_MOD_CONTROL) != 0;
}

bool Key_hasOption(int keyEvent) {
    return (keyEvent & KEY_MOD_OPTION) != 0;
}

bool Key_hasCommand(int keyEvent) {
    return (keyEvent & KEY_MOD_COMMAND) != 0;
}

const char *Key_name(int keyCode) {
    if (keyCode < 0 || keyCode >= KEY_COUNT) return "";
    const char *name = s_names[keyCode];
    return name ? name : "";
}
