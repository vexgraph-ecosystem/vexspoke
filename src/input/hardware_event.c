#include "input/hardware_event.h"

#include <string.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: HardwareEvent
 * ============================================================================
 * Unified hardware event multiplexer: packages discrete device events
 * (piano keys, DJ turntables, trackpad gestures, gamepads) into one
 * fixed-size discriminated union packet with a type tag, device id, and
 * microsecond timestamp. The fixed layout keeps events cache-friendly and
 * trivially copyable across thread boundaries — the event pump hands whole
 * packets to consumers with no per-type allocation. Factory constructors
 * zero the packet first so unused union members never leak stale bytes.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: HardwareEvent (input/hardware_event.c)
 * LEVEL: L2 — Hardware Input Event Multiplexer
 * ============================================================================
 * Provides factory constructors packaging discrete hardware device events
 * into unified, cache-friendly event packets.
 *
 * STRUCT FIELDS (Mirroring input/hardware_event.h):
 * ----------------------------------------------------------------------------
 *   HardwareEvent {
 *     HardwareEventType type;   // event discriminator (piano/turntable/gesture/gamepad)
 *     uint32_t deviceId;        // originating device id
 *     uint64_t timestampUs;     // event timestamp in microseconds
 *     union { ... } data;       // per-type payload (PianoKeyEvent/TurntableEvent/GestureEvent/GamepadState)
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Constructors: (.h)
 *   - HardwareEvent_makePiano(devId, piano)
 *   - HardwareEvent_makeTurntable(devId, tt)
 *   - HardwareEvent_makeGesture(devId, gesture)
 *   - HardwareEvent_makeGamepad(devId, gamepad)
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
