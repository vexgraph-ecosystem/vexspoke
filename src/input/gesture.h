#ifndef INPUT_GESTURE_H
#define INPUT_GESTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"
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

static inline GestureEvent GestureEvent_2(GesturePhase phase, float magnificationDelta) {
    return (GestureEvent) { .phase = phase, .magnificationDelta = magnificationDelta };
}
#define GestureEvent(...) CONSTRUCTOR_DISPATCH(GestureEvent, __VA_ARGS__)
static inline void GestureEvent_setPhase(GestureEvent *event, GesturePhase phase) {
    if (event) (*event).phase = phase;
}
static inline void GestureEvent_setMagnificationDelta(GestureEvent *event, float delta) {
    if (event) (*event).magnificationDelta = delta;
}

typedef struct GestureState {
    float  currentScale;       // Absolute accumulated zoom scale (default 1.0)
    float  currentRotationRad; // Absolute accumulated rotation
    Vec2   focalPoint;
    Vec2   scrollVelocity;     // Inertia velocity decay
    bool   isPinching;
    bool   isRotating;
} GestureState;

GestureState GestureState_0(void);
#define GestureState(...) CONSTRUCTOR_DISPATCH(GestureState, __VA_ARGS__)
void GestureState_processEvent(GestureState *state, GestureEvent event);
void GestureState_tick(GestureState *state, float deltaSeconds);

#endif
