#include "input/piano_key.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "annotation/overview.h"

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
