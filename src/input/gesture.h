#ifndef INPUT_GESTURE_H
#define INPUT_GESTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lang/vec2.h"

// input/gesture.h — Multi-Touch Trackpad & Gesture Hardware Model.
//
// Models trackpad pinch magnification, two-finger rotation, momentum scroll
// inertia, and multi-finger directional swipes.

typedef enum GesturePhase {
    GESTURE_PHASE_NONE = 0,
    GESTURE_PHASE_BEGAN,
    GESTURE_PHASE_CHANGED,
    GESTURE_PHASE_ENDED,
    GESTURE_PHASE_CANCELLED,
    GESTURE_PHASE_MOMENTUM
} GesturePhase;

typedef enum SwipeDirection {
    SWIPE_NONE = 0,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN
} SwipeDirection;

typedef struct GestureEvent {
    GesturePhase   phase;
    Vec2           focalPoint;        // Normalized or pixel coordinate of gesture origin
    float          magnificationDelta;// Pinch scale delta (+ zoom in, - zoom out)
    float          rotationDeltaRad;  // Two-finger rotation angle in radians (+ CW, - CCW)
    Vec2           scrollDelta;       // Smooth scroll delta (horizontal & vertical)
    SwipeDirection swipe;             // Discrete multi-finger swipe if detected
    uint64_t       timestampUs;
} GestureEvent;

typedef struct GestureState {
    float  currentScale;       // Absolute accumulated zoom scale (default 1.0)
    float  currentRotationRad; // Absolute accumulated rotation
    Vec2   focalPoint;
    Vec2   scrollVelocity;     // Inertia velocity decay
    bool   isPinching;
    bool   isRotating;
} GestureState;

void GestureState_init(GestureState *state);
void GestureState_processEvent(GestureState *state, GestureEvent event);
void GestureState_tick(GestureState *state, float deltaSeconds);

#endif
