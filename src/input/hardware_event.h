#ifndef INPUT_HARDWARE_EVENT_H
#define INPUT_HARDWARE_EVENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "input/piano_key.h"
#include "input/turntable.h"
#include "input/gesture.h"
#include "input/gamepad.h"

// input/hardware_event.h — Unified Hardware Event Multiplexer.
//
// Unifies disparate hardware inputs (standard keyboards, musical piano keys,
// DJ turntables, trackpad multi-touch gestures, and game controllers) into
// a single, fixed-size discriminated event structure.

typedef enum HardwareEventType {
    HW_EVENT_NONE = 0,
    HW_EVENT_PIANO,
    HW_EVENT_TURNTABLE,
    HW_EVENT_GESTURE,
    HW_EVENT_GAMEPAD
} HardwareEventType;

typedef struct HardwareEvent {
    HardwareEventType type;
    uint32_t          deviceId;
    uint64_t          timestampUs;
    union {
        PianoKeyEvent  piano;
        TurntableEvent turntable;
        GestureEvent   gesture;
        GamepadState   gamepad;
    } data;
} HardwareEvent;

// Event constructors / factory helpers
HardwareEvent HardwareEvent_makePiano(uint32_t devId, PianoKeyEvent piano);
HardwareEvent HardwareEvent_makeTurntable(uint32_t devId, TurntableEvent tt);
HardwareEvent HardwareEvent_makeGesture(uint32_t devId, GestureEvent gesture);
HardwareEvent HardwareEvent_makeGamepad(uint32_t devId, GamepadState gamepad);

#endif
