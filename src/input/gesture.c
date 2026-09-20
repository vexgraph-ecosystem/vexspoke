#include "input/gesture.h"

#include <math.h>
#include <string.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Gesture
 * ============================================================================
 * Multi-touch trackpad gesture engine: folds raw trackpad events (pinch
 * magnification, two-finger rotation, momentum scroll, swipe directionality)
 * into an accumulated GestureState for high-precision creative viewports.
 * Exists because raw event deltas are noisy; the engine accumulates
 * scale/rotation with clamping and decays scroll velocity with exponential
 * friction. Memory: caller-owned GestureState value struct, zero allocation,
 * zero threads. Lifetime: caller-managed via GestureState_init;
 * processEvent/tick mutate the state in place.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Gesture (input/gesture.c)
 * LEVEL: L2 — Multi-Touch Trackpad Gesture Engine
 * ============================================================================
 * Processes trackpad pinch magnification, rotation, momentum scrolling,
 * and swipe directionality for high-precision creative viewports.
 *
 * STRUCT FIELDS (Mirroring input/gesture.h):
 * ----------------------------------------------------------------------------
 *   GestureEvent {
 *     GesturePhase phase;            // BEGAN / CHANGED / ENDED / CANCELLED / MOMENTUM
 *     Vec2 focalPoint;               // gesture origin (normalized or pixel coords)
 *     float magnificationDelta;      // pinch scale delta (+ zoom in, - zoom out)
 *     float rotationDeltaRad;        // two-finger rotation (+ CW, - CCW)
 *     Vec2 scrollDelta;              // smooth scroll delta (horizontal & vertical)
 *     SwipeDirection swipe;          // discrete multi-finger swipe if detected
 *     uint64_t timestampUs;          // event timestamp
 *   }
 *   GestureState {
 *     float currentScale;            // absolute accumulated zoom scale (default 1.0)
 *     float currentRotationRad;      // absolute accumulated rotation
 *     Vec2 focalPoint;               // gesture origin
 *     Vec2 scrollVelocity;           // inertia velocity decay
 *     bool isPinching;               // pinch in progress
 *     bool isRotating;               // rotation in progress
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - GestureState_init(state)
 *   - GestureState_processEvent(state, event)
 *   - GestureState_tick(state, deltaSeconds)
 * ============================================================================
 */

void GestureState_init(GestureState *state) {
    if (state == nullptr) return;
    memset(state, 0, sizeof(GestureState));
    (*state).currentScale = 1.0f;
    (*state).currentRotationRad = 0.0f;
    (*state).focalPoint.x = 0.0f;
    (*state).focalPoint.y = 0.0f;
    (*state).scrollVelocity.x = 0.0f;
    (*state).scrollVelocity.y = 0.0f;
    (*state).isPinching = false;
    (*state).isRotating = false;
}

void GestureState_processEvent(GestureState *state, GestureEvent event) {
    if (state == nullptr) return;

    (*state).focalPoint = event.focalPoint;

    switch (event.phase) {
        case GESTURE_PHASE_BEGAN:
            if (fabsf(event.magnificationDelta) > 0.0001f) {
                (*state).isPinching = true;
            }
            if (fabsf(event.rotationDeltaRad) > 0.0001f) {
                (*state).isRotating = true;
            }
            break;

        case GESTURE_PHASE_CHANGED:
            // Accumulate magnification
            (*state).currentScale *= (1.0f + event.magnificationDelta);
            if ((*state).currentScale < 0.05f) (*state).currentScale = 0.05f;
            if ((*state).currentScale > 50.0f) (*state).currentScale = 50.0f;

            // Accumulate rotation
            (*state).currentRotationRad += event.rotationDeltaRad;

            // Update scroll velocity
            (*state).scrollVelocity.horizontal = event.scrollDelta.horizontal;
            (*state).scrollVelocity.vertical = event.scrollDelta.vertical;
            break;

        case GESTURE_PHASE_ENDED:
        case GESTURE_PHASE_CANCELLED:
            (*state).isPinching = false;
            (*state).isRotating = false;
            break;

        case GESTURE_PHASE_MOMENTUM:
            // Momentum scroll tick
            (*state).scrollVelocity.horizontal = event.scrollDelta.horizontal;
            (*state).scrollVelocity.vertical = event.scrollDelta.vertical;
            break;

        default:
            break;
    }
}

void GestureState_tick(GestureState *state, float deltaSeconds) {
    if (state == nullptr || deltaSeconds <= 0.0f) return;

    // Decay momentum velocity with smooth exponential friction
    float friction = powf(0.1f, deltaSeconds);
    (*state).scrollVelocity.horizontal *= friction;
    (*state).scrollVelocity.vertical   *= friction;

    if (fabsf((*state).scrollVelocity.horizontal) < 0.001f) {
        (*state).scrollVelocity.horizontal = 0.0f;
    }
    if (fabsf((*state).scrollVelocity.vertical) < 0.001f) {
        (*state).scrollVelocity.vertical = 0.0f;
    }
}
