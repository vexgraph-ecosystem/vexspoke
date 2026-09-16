#include "input/gesture.h"

#include <math.h>
#include <string.h>
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Gesture (input/gesture.c)
 * LEVEL: L2 — Multi-Touch Trackpad Gesture Engine
 * ============================================================================
 * Processes trackpad pinch magnification, rotation, momentum scrolling,
 * and swipe directionality for high-precision creative viewports.
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
