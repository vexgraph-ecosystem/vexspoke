#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

#include "event/mousehandler.h"

// Window id carried by every queued event (0 = FOCUS_BROADCAST). windowId is
// an opaque OS tag, never a slot index: scoped registries grow on demand (the
// Dynamic Scalability & Anti-Hardcoding Law), so any id attaches.

// input/mouse.h — mouse buttons, position, and event stream
// (Legacy: input/Mouse.java).
//
// Same shape as key.h: a small off-heap state table (16 buttons), a packed
// 64-bit ring between Thread 0 producers and the game-thread dispatcher, and
// listener callbacks. Motion events carry coordinates instead of timestamps;
// button events carry timestamp + modifiers instead of coordinates.
//
// Click counting is windowed exactly like key taps: a press opens a click
// sequence whose pending window closes `tapWindowNanos` after the LAST press;
// while open the sequence is pending (Mouse_tapPhase), then it settles into
// SINGLE / DOUBLE / TRIPLE.
//
// Button-event wire format (identical to legacy):
//   [63:18] micros since engine start | [17:14] modifiers
//   [13: 2] button | [1:0] action (0 up | 1 down | 2 repeat)
// Motion events use the legacy marker bytes in bits [15:8]:
//   move: 255/5, delta: 255/9, zoom: 255/8, scroll: 254/6 — coords/float in
//   bits [47:16]; drag: real button in [15:8] with action 7.

enum {
    MOUSE_LEFT     = 0,
    MOUSE_RIGHT    = 1,
    MOUSE_MIDDLE   = 2,
    MOUSE_BUTTON_4 = 3,
    MOUSE_BUTTON_5 = 4,
    MOUSE_BUTTON_6 = 5,
    MOUSE_BUTTON_7 = 6,
    MOUSE_BUTTON_8 = 7,
};

void Mouse_init(void);
void Mouse_shutdown(void);
void Mouse_addListener(const MouseHandler *listener);
bool Mouse_removeListener(const MouseHandler *listener);

// Window-scoped registration (see input/key.h for the routing rules).
void Mouse_attachWindow(uint32_t windowId, const MouseHandler *listener);
bool Mouse_detachWindow(uint32_t windowId, const MouseHandler *listener);
void Mouse_detachWindowAll(uint32_t windowId);

// Producers: Thread 0 only. windowId tags the receiving window.
// tapWindowNanos gates click settlement (see the header comment above).
void Mouse_pushButtonEvent(uint32_t windowId, int button, int action, uint64_t tapWindowNanos);
void Mouse_pushMoveEvent(uint32_t windowId, double x, double y);
void Mouse_pushMoveDeltaEvent(uint32_t windowId, double dx, double dy);
void Mouse_pushDragEvent(uint32_t windowId, int button, double x, double y);
void Mouse_pushScrollEvent(uint32_t windowId, double dx, double dy);
void Mouse_pushZoomEvent(uint32_t windowId, double magnification);

// Consumer: drain the ring into the listeners. Game thread, once per frame.
void Mouse_dispatchEvents(void);

// --- State queries ---
bool     Mouse_isDown(int button);
uint64_t Mouse_pressTime(int button);
uint64_t Mouse_lastReleaseTime(int button);
uint64_t Mouse_lastHoldDurationNanos(int button);
uint64_t Mouse_currentHoldDurationNanos(int button);
int      Mouse_taps(int button);
void     Mouse_resetTaps(int button);

// --- Multi-tap gesture timeline (windowed settlement, mirrors KeyTapPhase) ---
typedef enum MouseTapPhase {
    MOUSE_TAP_NONE    = 0, // no clicks in flight (or consumed)
    MOUSE_TAP_PENDING = 1, // clicks counted, window still open — not yet settled
    MOUSE_TAP_SINGLE  = 2, // settled: window closed, taps == 1
    MOUSE_TAP_DOUBLE  = 3, // settled: window closed, taps == 2
    MOUSE_TAP_TRIPLE  = 4, // settled: window closed, taps >= 3
} MouseTapPhase;

// Settled-or-windowed click readout for a button.
MouseTapPhase Mouse_tapPhase(int button);

// One-shot LONG_PRESS latch (mirrors Key_isLongPressFired).
bool Mouse_isLongPressFired(int button);
void Mouse_setLongPressFired(int button, bool fired);

// Last dispatched cursor position in content coordinates (top-left origin).
// Written on dispatch of move/drag events — legacy left these dead; here they
// are fed so cameras/UI can poll without a listener.
double Mouse_x(void);
double Mouse_y(void);

// --- Packed mouseEvent helpers ---
int  Mouse_button(int mouseEvent);
bool Mouse_hasShift(int mouseEvent);
bool Mouse_hasControl(int mouseEvent);
bool Mouse_hasOption(int mouseEvent);
bool Mouse_hasCommand(int mouseEvent);

const char *Mouse_name(int button);

#endif
