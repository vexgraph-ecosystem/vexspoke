#include "input/turntable.h"

#include <math.h>
#include <string.h>
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Turntable (input/turntable.c)
 * LEVEL: L2 — DJ Turntable & Jog Wheel Physics Model
 * ============================================================================
 * Models vinyl scratch physics, platter angular momentum, pitch adjustment,
 * cue triggers, and crossfader curves for DJ performance software.
 * ============================================================================
 */

void TurntableState_init(TurntableState *state, uint8_t deckIndex) {
    if (state == nullptr) return;
    memset(state, 0, sizeof(TurntableState));
    (*state).deckIndex = deckIndex;
    (*state).platterTouched = false;
    (*state).isPlaying = false;
    (*state).targetRpm = TURNTABLE_RPM_33;
    (*state).currentAngularVelocityRadPerSec = 0.0f;
    (*state).pitchFader = 0.0f;
    (*state).playheadSeconds = 0.0f;
    (*state).cuePositionSeconds = 0.0f;
    for (int i = 0; i < 8; i++) {
        (*state).hotCues[i] = -1.0f;
    }
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

void Turntable_calculateCrossfade(float position, CrossfaderCurve curve, float *outLeft, float *outRight) {
    if (outLeft == nullptr || outRight == nullptr) return;

    // Clamp position to [-1.0, 1.0] (Left to Right)
    if (position < -1.0f) position = -1.0f;
    if (position > 1.0f) position = 1.0f;

    // Normalized [0.0, 1.0]
    float t = (position + 1.0f) * 0.5f;

    switch (curve) {
        case CROSSFADER_LINEAR:
            *outLeft  = 1.0f - t;
            *outRight = t;
            break;

        case CROSSFADER_SHARP_CUT:
            // Fast DJ scratch cut: full volume until tight center
            *outLeft  = (position >= 0.95f) ? 0.0f : 1.0f;
            *outRight = (position <= -0.95f) ? 0.0f : 1.0f;
            break;

        case CROSSFADER_SMOOTH_DIP:
        default: {
            // Equal-power crossfade (-3dB at center)
            float angle = t * (3.14159265f * 0.5f);
            *outLeft  = cosf(angle);
            *outRight = sinf(angle);
            break;
        }
    }
}
