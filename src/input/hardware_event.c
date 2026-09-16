#include "input/hardware_event.h"

#include <string.h>
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: HardwareEvent (input/hardware_event.c)
 * LEVEL: L2 — Hardware Input Event Multiplexer
 * ============================================================================
 * Provides factory constructors packaging discrete hardware device events
 * into unified, cache-friendly event packets.
 * ============================================================================
 */

HardwareEvent HardwareEvent_makePiano(uint32_t devId, PianoKeyEvent piano) {
    HardwareEvent ev;
    memset(&ev, 0, sizeof(HardwareEvent));
    ev.type = HW_EVENT_PIANO;
    ev.deviceId = devId;
    ev.timestampUs = piano.timestampUs;
    ev.data.piano = piano;
    return ev;
}

HardwareEvent HardwareEvent_makeTurntable(uint32_t devId, TurntableEvent tt) {
    HardwareEvent ev;
    memset(&ev, 0, sizeof(HardwareEvent));
    ev.type = HW_EVENT_TURNTABLE;
    ev.deviceId = devId;
    ev.timestampUs = tt.timestampUs;
    ev.data.turntable = tt;
    return ev;
}

HardwareEvent HardwareEvent_makeGesture(uint32_t devId, GestureEvent gesture) {
    HardwareEvent ev;
    memset(&ev, 0, sizeof(HardwareEvent));
    ev.type = HW_EVENT_GESTURE;
    ev.deviceId = devId;
    ev.timestampUs = gesture.timestampUs;
    ev.data.gesture = gesture;
    return ev;
}

HardwareEvent HardwareEvent_makeGamepad(uint32_t devId, GamepadState gamepad) {
    HardwareEvent ev;
    memset(&ev, 0, sizeof(HardwareEvent));
    ev.type = HW_EVENT_GAMEPAD;
    ev.deviceId = devId;
    ev.data.gamepad = gamepad;
    return ev;
}
