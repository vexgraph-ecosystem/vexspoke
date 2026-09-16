#ifndef INPUT_PIANO_KEY_H
#define INPUT_PIANO_KEY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// input/piano_key.h — Musical / MIDI Keyboard Pitch & Event Model.
//
// Represents musical keyboard input, MIDI note numbers (0..127), tuning frequencies,
// polyphonic key velocity, channel pressure, pitch bend, and sustain pedal.

#define PIANO_MAX_NOTES 128
#define PIANO_A4_NOTE   69
#define PIANO_A4_FREQ   440.0f

typedef enum PianoPitchClass {
    PITCH_C = 0,
    PITCH_CSHARP,
    PITCH_D,
    PITCH_DSHARP,
    PITCH_E,
    PITCH_F,
    PITCH_FSHARP,
    PITCH_G,
    PITCH_GSHARP,
    PITCH_A,
    PITCH_ASHARP,
    PITCH_B
} PianoPitchClass;

typedef struct PianoKeyEvent {
    uint8_t  note;        // 0..127 (MIDI standard; 60 = Middle C / C4)
    uint8_t  channel;     // 0..15 MIDI channel
    bool     isDown;      // true = Note On, false = Note Off
    float    velocity;    // Normalized strike velocity [0.0, 1.0]
    float    pressure;    // Polyphonic aftertouch [0.0, 1.0]
    float    pitchBend;   // [-1.0, +1.0]
    uint64_t timestampUs; // Microsecond timestamp
} PianoKeyEvent;

typedef struct PianoState {
    float    velocities[PIANO_MAX_NOTES]; // 0.0 if not pressed
    float    pressures[PIANO_MAX_NOTES];  // Aftertouch per key
    uint32_t activeNotesCount;
    float    pitchBend;                   // Current pitch wheel bend [-1.0, 1.0]
    float    modWheel;                    // CC 1 modulation [0.0, 1.0]
    bool     sustainPedal;                // CC 64 damper pedal
} PianoState;

// Note tuning and name calculations
float           PianoKey_frequency(uint8_t note);
static inline float PianoKey_equal_temperament_hz(uint8_t note) { return PianoKey_frequency(note); }
PianoPitchClass PianoKey_pitchClass(uint8_t note);
int32_t         PianoKey_octave(uint8_t note);
void            PianoKey_noteName(uint8_t note, char *dest, size_t maxLen);

// State container management
void PianoState_init(PianoState *state);
void PianoState_processEvent(PianoState *state, PianoKeyEvent event);
bool PianoState_isNoteOn(const PianoState *state, uint8_t note);
float PianoState_noteVelocity(const PianoState *state, uint8_t note);

#endif
