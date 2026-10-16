#ifndef INPUT_TURNTABLE_H
#define INPUT_TURNTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"

// input/turntable.h — DJ Turntable & Jog Wheel Hardware Model.
//
// Represents DJ jog wheel/platter interaction, vinyl scratch mode,
// platter angular velocity, pitch fader, cue buttons, and crossfader dynamics.

typedef enum TurntableRpm {
    TURNTABLE_RPM_33 = 0,
    TURNTABLE_RPM_45 = 1
} TurntableRpm;

typedef enum CrossfaderCurve {
    CROSSFADER_LINEAR = 0,
    CROSSFADER_SHARP_CUT,  // For DJ scratching (instant cut near edges)
    CROSSFADER_SMOOTH_DIP  // Equal-power crossfade (-3dB center)
} CrossfaderCurve;

typedef struct TurntableCrossfade {
    float left;
    float right;
} TurntableCrossfade;

typedef struct TurntableEvent {
    uint8_t  deckIndex;         // 0 = Deck A, 1 = Deck B, etc.
    bool     platterTouched;    // True if DJ's hand is contacting the touch platter
    bool     rimTouched;        // True if outer edge nudged
    float    angularDeltaRad;   // Radians rotated this tick (+ forward, - backward)
    float    pitchFader;        // Pitch / tempo slider [-1.0, +1.0] (0.0 = 0% pitch)
    bool     cuePressed;        // Main cue button
    bool     playPausePressed;  // Play/pause motor button
    uint8_t  hotCueIndex;       // 1..8 hot cue button triggered (0 = none)
    uint64_t timestampUs;
} TurntableEvent;

static inline TurntableEvent TurntableEvent_3(uint8_t deckIndex, bool platterTouched,
                                               float angularDeltaRadians) {
    return (TurntableEvent) { .deckIndex = deckIndex, .platterTouched = platterTouched,
        .angularDeltaRad = angularDeltaRadians };
}
#define TurntableEvent(...) CONSTRUCTOR_DISPATCH(TurntableEvent, __VA_ARGS__)
static inline void TurntableEvent_setPlatterTouched(TurntableEvent *event, bool touched) {
    if (event) (*event).platterTouched = touched;
}
static inline void TurntableEvent_setAngularDelta(TurntableEvent *event, float radians) {
    if (event) (*event).angularDeltaRad = radians;
}

typedef struct TurntableState {
    uint8_t      deckIndex;
    bool         platterTouched;     // In scratch mode when touched
    bool         isPlaying;          // Motor active
    TurntableRpm targetRpm;
    float        currentAngularVelocityRadPerSec;
    float        pitchFader;         // Current tempo slider
    float        playheadSeconds;
    float        cuePositionSeconds;
    float        hotCues[8];         // Saved hot cue positions in seconds (-1 = unset)
} TurntableState;

TurntableState TurntableState_1(uint8_t deckIndex);
#define TurntableState(...) CONSTRUCTOR_DISPATCH(TurntableState, __VA_ARGS__)
void TurntableState_processEvent(TurntableState *state, TurntableEvent event);
void TurntableState_tick(TurntableState *state, float deltaSeconds);

// Crossfader volume calculation; returns both channel volumes by value.
TurntableCrossfade Turntable_calculateCrossfade(float position, CrossfaderCurve curve);

#endif
