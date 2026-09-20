#include "input/piano_key.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: PianoKey
 * ============================================================================
 * Musical / MIDI keyboard pitch and event model. Converts MIDI note numbers
 * (0..127) to standard equal-temperament A440 frequencies, provides note
 * naming (e.g. C4, A4), and maintains polyphonic velocity and aftertouch
 * state. PianoState is a plain value struct with fixed 128-entry velocity and
 * pressure arrays plus pitch bend, mod wheel, and sustain pedal state; callers
 * own it on the stack or in an input slot. Note math is pure and allocation
 * free, so it is safe on hot input paths.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: PianoKey (input/piano_key.c)
 * LEVEL: L2 — Musical Input & Tuning Model
 * ============================================================================
 * Musical / MIDI Keyboard Pitch & Event Model.
 *
 * Converts MIDI note numbers (0..127) to standard equal temperament A440
 * frequencies, provides note naming (e.g. C4, A4), and maintains polyphonic
 * velocity and aftertouch state.
 *
 * STRUCT FIELDS (Mirroring input/piano_key.h):
 * ----------------------------------------------------------------------------
 *   PianoKeyEvent {
 *     uint8_t  note;        // 0..127 (MIDI standard; 60 = Middle C / C4)
 *     uint8_t  channel;     // 0..15 MIDI channel
 *     bool     isDown;      // true = Note On, false = Note Off
 *     float    velocity;    // normalized strike velocity [0.0, 1.0]
 *     float    pressure;    // polyphonic aftertouch [0.0, 1.0]
 *     float    pitchBend;   // [-1.0, +1.0]
 *     uint64_t timestampUs; // microsecond timestamp
 *   }
 *   PianoState {
 *     float    velocities[PIANO_MAX_NOTES]; // 0.0 if not pressed
 *     float    pressures[PIANO_MAX_NOTES];  // aftertouch per key
 *     uint32_t activeNotesCount;
 *     float    pitchBend;                   // current pitch wheel bend [-1.0, 1.0]
 *     float    modWheel;                    // CC 1 modulation [0.0, 1.0]
 *     bool     sustainPedal;                // CC 64 damper pedal
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - PianoKey_frequency(note)
 *   - PianoKey_pitchClass(note)
 *   - PianoKey_octave(note)
 *   - PianoKey_noteName(note, dest, maxLen)
 *   - PianoState_init(state)
 *   - PianoState_processEvent(state, event)
 *   - PianoState_isNoteOn(state, note)
 *   - PianoState_noteVelocity(state, note)
 * ============================================================================
 */

static const char *const PITCH_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

float PianoKey_frequency(uint8_t note) {
    if (note >= PIANO_MAX_NOTES) {
        note = 127;
    }
    // Equal temperament formula: f = 440 * 2^((note - 69) / 12)
    float semitoneOffset = (float) ((int32_t) note - PIANO_A4_NOTE);
    return PIANO_A4_FREQ * powf(2.0f, semitoneOffset / 12.0f);
}

PianoPitchClass PianoKey_pitchClass(uint8_t note) {
    return (PianoPitchClass) (note % 12);
}

int32_t PianoKey_octave(uint8_t note) {
    // MIDI 60 (Middle C) is C4. 60 / 12 = 5, minus 1 = 4.
    return ((int32_t) note / 12) - 1;
}

void PianoKey_noteName(uint8_t note, char *dest, size_t maxLen) {
    if (dest == nullptr || maxLen == 0) return;
    PianoPitchClass pc = PianoKey_pitchClass(note);
    int32_t oct = PianoKey_octave(note);
    snprintf(dest, maxLen, "%s%d", PITCH_NAMES[pc], oct);
}

void PianoState_init(PianoState *state) {
    if (state == nullptr) return;
    memset((*state).velocities, 0, sizeof((*state).velocities));
    memset((*state).pressures, 0, sizeof((*state).pressures));
    (*state).activeNotesCount = 0;
    (*state).pitchBend = 0.0f;
    (*state).modWheel = 0.0f;
    (*state).sustainPedal = false;
}

void PianoState_processEvent(PianoState *state, PianoKeyEvent event) {
    if (state == nullptr || event.note >= PIANO_MAX_NOTES) {
        return;
    }

    uint8_t n = event.note;
    bool wasDown = ((*state).velocities[n] > 0.0f);

    if (event.isDown) {
        (*state).velocities[n] = (event.velocity > 0.0f) ? event.velocity : 0.8f;
        (*state).pressures[n] = event.pressure;
        if (!wasDown) {
            (*state).activeNotesCount++;
        }
    } else {
        (*state).velocities[n] = 0.0f;
        (*state).pressures[n] = 0.0f;
        if (wasDown && (*state).activeNotesCount > 0) {
            (*state).activeNotesCount--;
        }
    }

    (*state).pitchBend = event.pitchBend;
}

bool PianoState_isNoteOn(const PianoState *state, uint8_t note) {
    if (state == nullptr || note >= PIANO_MAX_NOTES) return false;
    return (*state).velocities[note] > 0.0f;
}

float PianoState_noteVelocity(const PianoState *state, uint8_t note) {
    if (state == nullptr || note >= PIANO_MAX_NOTES) return 0.0f;
    return (*state).velocities[note];
}
