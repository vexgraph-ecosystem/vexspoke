// input/mouse.c — mouse buttons, position, and event stream
// (Legacy: input/Mouse.java port).
//
// Button slots mirror the Key state table (same 40-byte layout): 16 x 40-byte
// off-heap slots, one per button, with windowed-tap settlement — clicks are
// OFFERED while `now < pendingUntil` and settle (SINGLE/DOUBLE/TRIPLE) once
// the tap window closes. longPressFired is the one-shot hold latch.
//
// Wire format is byte-for-byte the legacy packing; the dispatcher classifies
// by marker byte FIRST (bits [15:8]) so button payloads can never collide
// with the motion markers (legacy read the low action nibble too late and
// silently dropped right-down / middle-up — fixed here):
//   marker 255 => move(5) / delta(9) / zoom(8)   marker 254 => scroll(6)
//   action8 == 7 => drag (marker holds the real button)
//   otherwise    => button event (action = packed & 3)
//
// Position is written on dispatch (move/drag), so Mouse_x()/Mouse_y() are
// live polls instead of the dead getters the legacy shipped.

#include "input/mouse.h"

#include <string.h>

#include "input/focus.h"
#include "input/key.h"
#include "atomic/ring.h"
#include "time/nanotime.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Mouse (input/mouse.c)
 * LEVEL: L2 — Behavior (input behavior API)
 * ============================================================================
 * mouse buttons, position, and event stream. Listener registries grow
 * exponentially (the Dynamic Scalability & Anti-Hardcoding Law): window scope
 * is a row keyed by the opaque OS window id, arena-backed, doubling on demand.
 *
 * STRUCT FIELDS (local to this file):
 * ----------------------------------------------------------------------------
 *   InputEvent {
 *     uint64_t packed;           // legacy 64-bit mouse event packing
 *     uint32_t windowId;         // target window (0 = broadcast)
 *     uint32_t pad;              // alignment padding
 *   }
 *   ButtonSlot {
 *     uint64_t pressTime;        // last press timestamp
 *     uint64_t lastReleaseTime;  // last release timestamp
 *     uint64_t pendingUntil;     // click window close; clicks settle once now >= pendingUntil
 *     uint64_t lastHoldDuration; // hold length accumulator
 *     int32_t taps;              // click-count state
 *     uint8_t lastTapMods;       // wire modifier mask at last press (change breaks sequence)
 *     uint8_t longPressFired;    // one-shot LONG_PRESS latch, cleared on release
 *     uint8_t pad[2];            // alignment padding
 *   }
 *
 * PRIVATE HELPERS (kept file-local pure-data only, each with full fields):
 * ----------------------------------------------------------------------------
 *   WinRow {
 *     uint32_t windowId;         // opaque OS window tag (0 never attached)
 *     const MouseHandler **items;  // arena-allocated listener segment, doubling
 *     int count;                 // live listener count
 *     int cap;                   // allocated segment capacity
 *   }
 *   (growSegment / growRows / rowFor: static behavior, zero struct fields)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:
 *   - Mouse_init(void)
 *
 * Core Functions:
 *   - Mouse_shutdown(void)
 *   - Mouse_addListener(listener)
 *   - Mouse_removeListener(listener)
 *   - Mouse_attachWindow(windowId, listener)
 *   - Mouse_detachWindow(windowId, listener)
 *   - Mouse_detachWindowAll(windowId)
 *   - Mouse_pushButtonEvent(windowId, button, action, tapWindowNanos)
 *   - Mouse_pushMoveEvent(windowId, x, y)
 *   - Mouse_pushMoveDeltaEvent(windowId, dx, dy)
 *   - Mouse_pushDragEvent(windowId, button, x, y)
 *   - Mouse_pushScrollEvent(windowId, dx, dy)
 *   - Mouse_pushZoomEvent(windowId, magnification)
 *   - Mouse_dispatchEvents(void)
 *   - Mouse_pressTime(button)
 *   - Mouse_lastReleaseTime(button)
 *   - Mouse_lastHoldDurationNanos(button)
 *   - Mouse_currentHoldDurationNanos(button)
 *   - Mouse_taps(button)
 *   - Mouse_resetTaps(button)
 *   - Mouse_tapPhase(button)
 *   - Mouse_isLongPressFired(button)
 *   - Mouse_setLongPressFired(button, fired)
 *   - Mouse_x(void)
 *   - Mouse_y(void)
 *   - Mouse_button(mouseEvent)
 *   - Mouse_name(button)
 *
 * Getters:
 *   - Mouse_isDown(button)
 *   - Mouse_hasShift(mouseEvent)
 *   - Mouse_hasControl(mouseEvent)
 *   - Mouse_hasOption(mouseEvent)
 *   - Mouse_hasCommand(mouseEvent)
 * ============================================================================
 */


#define BUTTON_COUNT 16
;;INTENTION("fixed BUTTON_COUNT: the mouse-button domain is a hardware enum (0..15), not a workload ceiling")

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
    uint64_t pendingUntil;     // click window close; clicks settle once now >= pendingUntil
    uint64_t lastHoldDuration;
    int32_t taps;
    uint8_t lastTapMods;       // wire modifier mask at last press (change breaks sequence)
    uint8_t longPressFired;    // one-shot LONG_PRESS latch, cleared on release
    uint8_t pad[2];            // alignment padding
} ButtonSlot;

_Static_assert(sizeof(ButtonSlot) == 40, "button slot must stay 40 bytes");

static ButtonSlot s_slots[BUTTON_COUNT];
static double s_posX = 0.0;
static double s_posY = 0.0;
static RingBuffer s_queue;
static bool s_ready = false;

// --- PRIVATE HELPERS: growable listener registries ---
// The Dynamic Scalability & Anti-Hardcoding Law: registries start at zero
// allocations and double exponentially, arena-backed. windowId is an opaque
// OS tag (0 = FOCUS_BROADCAST, reserved, never attached); broadcast events
// fan _out to every attached row.
typedef struct WinRow {
    uint32_t windowId;
    const MouseHandler **items;  // arena-allocated segment, doubling
    int count;
    int cap;
} WinRow;

static const MouseHandler **s_listeners = NULL;  // global listener segment
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
static bool growSegment(const MouseHandler ***items, int *cap, int needed) {
    if (needed <= *cap) return true;
    int newCap = (*cap == 0) ? 16 : *cap * 2;
    while (newCap < needed) newCap *= 2;
    const MouseHandler **nb = (const MouseHandler**) Memory_alloc(
        TYPE_INT_POINTER, (size_t) newCap * sizeof(MouseHandler *));
    if (nb == NULL) return false;
    if (*items != NULL && *cap > 0)
        memcpy(nb, *items, (size_t) *cap * sizeof(MouseHandler *));
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

void Mouse_init(void) {
    if (s_ready) return;
    NanoTime_init();
    if (!RingBuffer_init(&s_queue, sizeof(InputEvent), QUEUE_CAPACITY)) return;
    memset(s_slots, 0, sizeof(s_slots));
    s_ready = true;
}

void Mouse_shutdown(void) {
    if (!s_ready) return;
    RingBuffer_shutdown(&s_queue);
    s_ready = false;
}

void Mouse_addListener(const MouseHandler *listener) {
    if (listener == NULL) return;
    if (!growSegment(&s_listeners, &s_listenerCap, s_listenerCount + 1)) return;
    s_listeners[s_listenerCount++] = listener;
}

bool Mouse_removeListener(const MouseHandler *listener) {
    for (int i = 0; i < s_listenerCount; i++) {
        if (s_listeners[i] == listener) {
            s_listeners[i] = s_listeners[--s_listenerCount];
            return true;
        }
    }
    return false;
}

void Mouse_attachWindow(uint32_t windowId, const MouseHandler *listener) {
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

bool Mouse_detachWindow(uint32_t windowId, const MouseHandler *listener) {
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

void Mouse_detachWindowAll(uint32_t windowId) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row != NULL)
        (*row).count = 0;
}

static int modifierMask(void) {
    int mods = 0;
    if (Key_isDown(KEY_LEFT_SHIFT) || Key_isDown(KEY_RIGHT_SHIFT)) mods |= 1;
    if (Key_isDown(KEY_LEFT_CONTROL) || Key_isDown(KEY_RIGHT_CONTROL)) mods |= 2;
    if (Key_isDown(KEY_LEFT_ALT) || Key_isDown(KEY_RIGHT_ALT)) mods |= 4;
    if (Key_isDown(KEY_LEFT_SUPER) || Key_isDown(KEY_RIGHT_SUPER)) mods |= 8;
    return mods;
}

static uint64_t epochMicros(void) {
    return (NanoTime_elapsedNanos() / 1000ULL) & 0x3FFFFFFFFFFFULL;
}

void Mouse_pushButtonEvent(uint32_t windowId, int button, int action, uint64_t tapWindowNanos) {
    if (button < 0 || button >= BUTTON_COUNT) return;
    if (!s_ready) Mouse_init();
    if (!s_ready) return;

    ButtonSlot *slot = &s_slots[button];
    uint64_t now = NanoTime_now();

    if (action == KEY_ACTION_DOWN) {
        if ((*slot).pressTime != 0) { // OS repeat
            InputEvent ev = { .packed = (epochMicros() << 18)
                                       | (((uint64_t)modifierMask() & 0xF) << 14)
                                       | (((uint64_t)button & 0xFFF) << 2)
                                       | KEY_ACTION_REPEAT,
                              .windowId = windowId, .pad = 0 };
            RingBuffer_push(&s_queue, &ev);
            return;
        }
        uint64_t lastRelease = (*slot).lastReleaseTime;
        // A modifier-state change since the last press breaks the sequence:
        // different modifiers => a fresh click, never an upgrade to double.
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
                               | (((uint64_t)button & 0xFFF) << 2)
                               | ((uint64_t)action & 0x3),
                      .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

// Coordinates ride as int16 halves exactly like legacy (wrapping truncation).
static uint64_t packCoords(uint64_t head, double x, double y) {
    uint64_t packed = head;
    packed |= (((uint64_t)(uint16_t)(int16_t)(int)x) << 16);
    packed |= (((uint64_t)(uint16_t)(int16_t)(int)y) << 32);
    return packed;
}

static double coordA(uint64_t packed) {
    return (double)(int16_t)((packed >> 16) & 0xFFFF);
}

static double coordB(uint64_t packed) {
    return (double)(int16_t)((packed >> 32) & 0xFFFF);
}

void Mouse_pushMoveEvent(uint32_t windowId, double x, double y) {
    if (!s_ready) Mouse_init();
    if (!s_ready) return;
    InputEvent ev = { .packed = packCoords((255ULL << 8) | 5ULL, x, y), .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

void Mouse_pushMoveDeltaEvent(uint32_t windowId, double dx, double dy) {
    if (!s_ready) Mouse_init();
    if (!s_ready) return;
    InputEvent ev = { .packed = packCoords((255ULL << 8) | 9ULL, dx, dy), .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

void Mouse_pushDragEvent(uint32_t windowId, int button, double x, double y) {
    if (!s_ready) Mouse_init();
    if (!s_ready) return;
    InputEvent ev = { .packed = packCoords(((uint64_t)button << 8) | 7ULL, x, y), .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

void Mouse_pushScrollEvent(uint32_t windowId, double dx, double dy) {
    if (!s_ready) Mouse_init();
    if (!s_ready) return;

    // Scroll rides at 100x resolution inside an int16 per axis.
    double sx = dx * 100.0;
    double sy = dy * 100.0;
    if (sx > 32767.0) sx = 32767.0;
    if (sx < -32768.0) sx = -32768.0;
    if (sy > 32767.0) sy = 32767.0;
    if (sy < -32768.0) sy = -32768.0;

    uint64_t packed = (254ULL << 8) | 6ULL;
    packed |= ((uint64_t)(uint16_t)(int16_t)sx) << 16;
    packed |= ((uint64_t)(uint16_t)(int16_t)sy) << 32;
    InputEvent ev = { .packed = packed, .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

void Mouse_pushZoomEvent(uint32_t windowId, double magnification) {
    if (!s_ready) Mouse_init();
    if (!s_ready) return;

    float f = (float)magnification;
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));

    uint64_t packed = (255ULL << 8) | 8ULL;
    packed |= ((uint64_t)bits & 0xFFFFFFFFULL) << 16;
    InputEvent ev = { .packed = packed, .windowId = windowId, .pad = 0 };
    RingBuffer_push(&s_queue, &ev);
}

// Deliver one motion-class event (move/delta/zoom/scroll/drag) to one
// window's scoped list.
static void deliverMotion(uint32_t windowId, int action8, int button,
                          double a, double b) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row == NULL) return;
    for (int i = 0; i < (*row).count; i++) {
        const MouseHandler *l = (*row).items[i];
        void *self = (*l).self;
        if (action8 == 5 && (*l).onMouseMove)
            (*l).onMouseMove(self, a, b);
        else if (action8 == 9 && (*l).onMouseMoveDelta)
            (*l).onMouseMoveDelta(self, a, b);
        else if (action8 == 6 && (*l).onMouseScroll)
            (*l).onMouseScroll(self, a, b);
        else if (action8 == 7 && (*l).onMouseDrag)
            (*l).onMouseDrag(self, button, a, b);
    }
}

// Zoom rides the same routing but carries float bits instead of coords.
static void deliverZoom(uint32_t windowId, double magnification) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row == NULL) return;
    for (int i = 0; i < (*row).count; i++) {
        const MouseHandler *l = (*row).items[i];
        if ((*l).onMouseZoom)
            (*l).onMouseZoom((*l).self, magnification);
    }
}

// Deliver one button-class event to one window's scoped list.
static void deliverButton(uint32_t windowId, int action, int mouseEvent, uint64_t exactNanos) {
    if (windowId == 0) return;
    WinRow *row = rowFor(windowId);
    if (row == NULL) return;
    for (int i = 0; i < (*row).count; i++) {
        const MouseHandler *l = (*row).items[i];
        void *self = (*l).self;
        if (action == KEY_ACTION_DOWN && (*l).onMouseDown)
            (*l).onMouseDown(self, mouseEvent, exactNanos);
        else if (action == KEY_ACTION_UP && (*l).onMouseUp)
            (*l).onMouseUp(self, mouseEvent, exactNanos);
        else if (action == KEY_ACTION_REPEAT && (*l).onMouseRepeat)
            (*l).onMouseRepeat(self, mouseEvent, exactNanos);
    }
}

void Mouse_dispatchEvents(void) {
    if (!s_ready) return;

    InputEvent ev;
    while (RingBuffer_pop(&s_queue, &ev)) {
        uint64_t packed = ev.packed;
        int action8 = (int)(packed & 0xFF);
        int marker = (int)((packed >> 8) & 0xFF);

        // --- Motion class: marker bytes 254/255 ---
        if (marker == 255) {
            if (action8 == 5) { // move
                s_posX = coordA(packed);
                s_posY = coordB(packed);
                for (int i = 0; i < s_listenerCount; i++)
                    if ((*s_listeners[i]).onMouseMove)
                        (*s_listeners[i]).onMouseMove((*s_listeners[i]).self, s_posX, s_posY);
                if (ev.windowId == FOCUS_BROADCAST) {
                    for (int r = 0; r < s_rowCount; r++)
                        deliverMotion(s_rows[r].windowId, 5, 0, s_posX, s_posY);
                } else {
                    deliverMotion(ev.windowId, 5, 0, s_posX, s_posY);
                }
            } else if (action8 == 9) { // locked-cursor delta
                double dx = coordA(packed);
                double dy = coordB(packed);
                for (int i = 0; i < s_listenerCount; i++)
                    if ((*s_listeners[i]).onMouseMoveDelta)
                        (*s_listeners[i]).onMouseMoveDelta((*s_listeners[i]).self, dx, dy);
                if (ev.windowId == FOCUS_BROADCAST) {
                    for (int r = 0; r < s_rowCount; r++)
                        deliverMotion(s_rows[r].windowId, 9, 0, dx, dy);
                } else {
                    deliverMotion(ev.windowId, 9, 0, dx, dy);
                }
            } else if (action8 == 8) { // pinch zoom
                uint32_t bits = (uint32_t)((packed >> 16) & 0xFFFFFFFFULL);
                float f;
                memcpy(&f, &bits, sizeof(f));
                double mag = (double)f;
                for (int i = 0; i < s_listenerCount; i++)
                    if ((*s_listeners[i]).onMouseZoom)
                        (*s_listeners[i]).onMouseZoom((*s_listeners[i]).self, mag);
                if (ev.windowId == FOCUS_BROADCAST) {
                    for (int r = 0; r < s_rowCount; r++)
                        deliverZoom(s_rows[r].windowId, mag);
                } else {
                    deliverZoom(ev.windowId, mag);
                }
            }
            continue;
        }
        if (marker == 254 && action8 == 6) { // scroll
            double dx = coordA(packed) / 100.0;
            double dy = coordB(packed) / 100.0;
            for (int i = 0; i < s_listenerCount; i++)
                if ((*s_listeners[i]).onMouseScroll)
                    (*s_listeners[i]).onMouseScroll((*s_listeners[i]).self, dx, dy);
            if (ev.windowId == FOCUS_BROADCAST) {
                for (int r = 0; r < s_rowCount; r++)
                    deliverMotion(s_rows[r].windowId, 6, 0, dx, dy);
            } else {
                deliverMotion(ev.windowId, 6, 0, dx, dy);
            }
            continue;
        }

        // --- Drag: real button in the marker byte, action 7 ---
        if (action8 == 7) {
            s_posX = coordA(packed);
            s_posY = coordB(packed);
            int button = marker;
            for (int i = 0; i < s_listenerCount; i++)
                if ((*s_listeners[i]).onMouseDrag)
                    (*s_listeners[i]).onMouseDrag((*s_listeners[i]).self, button, s_posX, s_posY);
            if (ev.windowId == FOCUS_BROADCAST) {
                for (int r = 0; r < s_rowCount; r++)
                    deliverMotion(s_rows[r].windowId, 7, button, s_posX, s_posY);
            } else {
                deliverMotion(ev.windowId, 7, button, s_posX, s_posY);
            }
            continue;
        }

        // --- Button class: timestamped, modifier-tagged ---
        int action = (int)(packed & 0x3);
        int button = (int)((packed >> 2) & 0xFFF);
        int modifiers = (int)((packed >> 14) & 0xF);
        uint64_t exactNanos = NanoTime_startNanos()
                            + (((packed >> 18) & 0x3FFFFFFFFFFFULL) * 1000ULL);

        int mappedMods = 0;
        if ((modifiers & 1) != 0) mappedMods |= KEY_MOD_SHIFT;
        if ((modifiers & 2) != 0) mappedMods |= KEY_MOD_CONTROL;
        if ((modifiers & 4) != 0) mappedMods |= KEY_MOD_OPTION;
        if ((modifiers & 8) != 0) mappedMods |= KEY_MOD_COMMAND;
        int mouseEvent = button | mappedMods;

        for (int i = 0; i < s_listenerCount; i++) {
            void *self = (*s_listeners[i]).self;
            if (action == KEY_ACTION_DOWN && (*s_listeners[i]).onMouseDown)
                (*s_listeners[i]).onMouseDown(self, mouseEvent, exactNanos);
            else if (action == KEY_ACTION_UP && (*s_listeners[i]).onMouseUp)
                (*s_listeners[i]).onMouseUp(self, mouseEvent, exactNanos);
            else if (action == KEY_ACTION_REPEAT && (*s_listeners[i]).onMouseRepeat)
                (*s_listeners[i]).onMouseRepeat(self, mouseEvent, exactNanos);
        }
        if (ev.windowId == FOCUS_BROADCAST) {
            for (int r = 0; r < s_rowCount; r++)
                deliverButton(s_rows[r].windowId, action, mouseEvent, exactNanos);
        } else {
            deliverButton(ev.windowId, action, mouseEvent, exactNanos);
        }
    }
}

bool Mouse_isDown(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return false;
    return s_slots[button].pressTime != 0;
}

uint64_t Mouse_pressTime(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return 0;
    return s_slots[button].pressTime;
}

uint64_t Mouse_lastReleaseTime(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return 0;
    return s_slots[button].lastReleaseTime;
}

uint64_t Mouse_lastHoldDurationNanos(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return 0;
    return s_slots[button].lastHoldDuration;
}

uint64_t Mouse_currentHoldDurationNanos(int button) {
    uint64_t p = Mouse_pressTime(button);
    return p == 0 ? 0 : NanoTime_now() - p;
}

int Mouse_taps(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return 0;
    return s_slots[button].taps;
}

void Mouse_resetTaps(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return;
    s_slots[button].taps = 0;
}

MouseTapPhase Mouse_tapPhase(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return MOUSE_TAP_NONE;
    const ButtonSlot *slot = &s_slots[button];
    if ((*slot).taps == 0)
        return MOUSE_TAP_NONE;
    if (NanoTime_now() < (*slot).pendingUntil)
        return MOUSE_TAP_PENDING;
    if ((*slot).taps >= 3)
        return MOUSE_TAP_TRIPLE;
    if ((*slot).taps == 2)
        return MOUSE_TAP_DOUBLE;
    return MOUSE_TAP_SINGLE;
}

bool Mouse_isLongPressFired(int button) {
    if (button < 0 || button >= BUTTON_COUNT) return false;
    return s_slots[button].longPressFired != 0;
}

void Mouse_setLongPressFired(int button, bool fired) {
    if (button < 0 || button >= BUTTON_COUNT) return;
    s_slots[button].longPressFired = (uint8_t)(fired ? 1 : 0);
}

double Mouse_x(void) {
    return s_posX;
}

double Mouse_y(void) {
    return s_posY;
}

int Mouse_button(int mouseEvent) {
    return mouseEvent & 0xFFFF;
}

bool Mouse_hasShift(int mouseEvent) {
    return (mouseEvent & KEY_MOD_SHIFT) != 0;
}

bool Mouse_hasControl(int mouseEvent) {
    return (mouseEvent & KEY_MOD_CONTROL) != 0;
}

bool Mouse_hasOption(int mouseEvent) {
    return (mouseEvent & KEY_MOD_OPTION) != 0;
}

bool Mouse_hasCommand(int mouseEvent) {
    return (mouseEvent & KEY_MOD_COMMAND) != 0;
}

const char *Mouse_name(int button) {
    switch (button) {
        case MOUSE_LEFT: return "Left";
        case MOUSE_RIGHT: return "Right";
        case MOUSE_MIDDLE: return "Middle";
        default: return "Button";
    }
}
