#include "input/turntable.h"

#include <math.h>
#include <string.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Turntable
 * ============================================================================
 * DJ turntable and jog wheel physics model: vinyl scratch platter state,
 * motor inertia, pitch fader, cue points, hot cues, and crossfader curves.
 * TurntableState is a caller-owned struct advanced by processEvent (discrete
 * hardware input) and tick (continuous motor simulation with smooth
 * acceleration and deceleration friction). The model is deterministic and
 * allocation-free — playheadSeconds integrates angular velocity so scratch
 * deltas and motor motion compose on the same timeline. Crossfader curves
 * are pure functions returning both channel volumes by value.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Turntable (input/turntable.c)
 * ============================================================================
 * Models vinyl scratch physics, platter angular momentum, pitch adjustment,
 * cue triggers, and crossfader curves for DJ performance software.
 *
 * STRUCT FIELDS (Mirroring input/turntable.h):
 * ----------------------------------------------------------------------------
 *   TurntableState {
 *     uint8_t deckIndex;                      // 0 = Deck A, 1 = Deck B, etc.
 *     bool platterTouched;                    // in scratch mode when touched
 *     bool isPlaying;                         // motor active
 *     TurntableRpm targetRpm;                 // 33 or 45 RPM target
 *     float currentAngularVelocityRadPerSec;  // platter angular velocity
 *     float pitchFader;                       // current tempo slider
 *     float playheadSeconds;                  // track position
 *     float cuePositionSeconds;               // saved cue point
 *     float hotCues[8];                       // saved hot cue positions (-1 = unset)
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - TurntableState(deckIndex)
 *   - TurntableState_processEvent(state, event)
 *   - TurntableState_tick(state, deltaSeconds)
 *   - Turntable_calculateCrossfade(position, curve)
 * ============================================================================
 */

TurntableState TurntableState_1(uint8_t deckIndex) {
    TurntableState state;
    memset(&state, 0, sizeof state);
    state.deckIndex = deckIndex;
    state.platterTouched = false;
    state.isPlaying = false;
    state.targetRpm = TURNTABLE_RPM_33;
    state.currentAngularVelocityRadPerSec = 0.0f;
    state.pitchFader = 0.0f;
    state.playheadSeconds = 0.0f;
    state.cuePositionSeconds = 0.0f;
    for (int i = 0; i < 8; i++) {
        state.hotCues[i] = -1.0f;
    }
    return state;
}

void TurntableState_processEvent(TurntableState *state, TurntableEvent event) {
    if (state == nullptr) return;

    (*state).platterTouched = event.platterTouched;
    (*state).pitchFader = event.pitchFader;

    if (event.playPausePressed) {
        (*state).isPlaying = !(*state).isPlaying;
    }

    if (event.cuePressed) {
        if (!(*state).isPlaying) {
            // Set cue point at current playhead
            (*state).cuePositionSeconds = (*state).playheadSeconds;
        } else {
            // Jump back to cue point and pause
            (*state).playheadSeconds = (*state).cuePositionSeconds;
            (*state).isPlaying = false;
        }
    }

    if (event.hotCueIndex >= 1 && event.hotCueIndex <= 8) {
        int idx = (int) event.hotCueIndex - 1;
        if ((*state).hotCues[idx] < 0.0f) {
            // Unset: save current position
            (*state).hotCues[idx] = (*state).playheadSeconds;
        } else {
            // Jump to saved hot cue
            (*state).playheadSeconds = (*state).hotCues[idx];
        }
    }

    if (event.platterTouched) {
        // Direct scratch delta overrides natural motor velocity
        // e.g. 1 revolution = 2*PI radians. Time delta ~16ms (60fps)
        (*state).currentAngularVelocityRadPerSec = event.angularDeltaRad / 0.01667f;
        // 33.33 RPM = 0.5555 rev/sec = ~3.4906 rad/sec
        float speedRatio = (*state).currentAngularVelocityRadPerSec / 3.49066f;
        (*state).playheadSeconds += speedRatio * 0.01667f;
    } else if (event.rimTouched) {
        // Nudge pitch momentarily
        (*state).playheadSeconds += (event.angularDeltaRad / 3.49066f);
    }
}

void TurntableState_tick(TurntableState *state, float deltaSeconds) {
    if (state == nullptr || deltaSeconds <= 0.0f) return;

    if (!(*state).platterTouched && (*state).isPlaying) {
        // Base nominal velocity
        float baseRpm = ((*state).targetRpm == TURNTABLE_RPM_45) ? 45.0f : 33.333f;
        float baseRadPerSec = baseRpm * (2.0f * 3.14159265f / 60.0f);

        // Pitch scale: 0.0 = 100%, +0.08 = 108% (8% pitch range standard)
        float pitchMultiplier = 1.0f + ((*state).pitchFader * 0.08f);
        float targetVelocity = baseRadPerSec * pitchMultiplier;

        // Smooth motor inertia acceleration
        float diff = targetVelocity - (*state).currentAngularVelocityRadPerSec;
        (*state).currentAngularVelocityRadPerSec += diff * (deltaSeconds * 8.0f);

        // Advance playhead
        float speedRatio = (*state).currentAngularVelocityRadPerSec / baseRadPerSec;
        (*state).playheadSeconds += speedRatio * deltaSeconds;
    } else if (!(*state).isPlaying && !(*state).platterTouched) {
        // Motor deceleration friction
        (*state).currentAngularVelocityRadPerSec *= powf(0.05f, deltaSeconds);
        if (fabsf((*state).currentAngularVelocityRadPerSec) < 0.01f) {
            (*state).currentAngularVelocityRadPerSec = 0.0f;
        }
    }

    if ((*state).playheadSeconds < 0.0f) {
        (*state).playheadSeconds = 0.0f;
    }
}

TurntableCrossfade Turntable_calculateCrossfade(float position, CrossfaderCurve curve) {
    // Clamp position to [-1.0, 1.0] (Left to Right)
    if (position < -1.0f) position = -1.0f;
    if (position > 1.0f) position = 1.0f;

    // Normalized [0.0, 1.0]
    float t = (position + 1.0f) * 0.5f;
    TurntableCrossfade volumes;

    switch (curve) {
        case CROSSFADER_LINEAR:
            volumes.left = 1.0f - t;
            volumes.right = t;
            break;

        case CROSSFADER_SHARP_CUT:
            // Fast DJ scratch cut: full volume until tight center
            volumes.left = (position >= 0.95f) ? 0.0f : 1.0f;
            volumes.right = (position <= -0.95f) ? 0.0f : 1.0f;
            break;

        case CROSSFADER_SMOOTH_DIP:
        default: {
            // Equal-power crossfade (-3dB at center)
            float angle = t * (3.14159265f * 0.5f);
            volumes.left = cosf(angle);
            volumes.right = sinf(angle);
            break;
        }
    }
    return volumes;
}
