#ifndef INPUT_TURNTABLE_H
#define INPUT_TURNTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

void TurntableState_init(TurntableState *state, uint8_t deckIndex);
void TurntableState_processEvent(TurntableState *state, TurntableEvent event);
void TurntableState_tick(TurntableState *state, float deltaSeconds);

// Crossfader volume calculation: dest-last order (writes volumeLeft and volumeRight)
void Turntable_calculateCrossfade(float position, CrossfaderCurve curve, float *outLeft, float *outRight);

#endif
