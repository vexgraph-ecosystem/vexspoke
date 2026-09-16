#ifndef INPUT_GAMEPAD_H
#define INPUT_GAMEPAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lang/vec2.h"

// input/gamepad.h — Game Controller & Joystick Hardware Model.
//
// Models dual analog thumbsticks with circular deadzone filtering,
// analog triggers, standard digital buttons, and dual-motor haptics.

#define GAMEPAD_DEFAULT_DEADZONE 0.15f

// Digital button bitmask
enum {
    GAMEPAD_BUTTON_A            = (1u << 0),  // Cross / Action 1
    GAMEPAD_BUTTON_B            = (1u << 1),  // Circle / Action 2
    GAMEPAD_BUTTON_X            = (1u << 2),  // Square / Action 3
    GAMEPAD_BUTTON_Y            = (1u << 3),  // Triangle / Action 4
    GAMEPAD_BUTTON_L1           = (1u << 4),  // Left Bumper
    GAMEPAD_BUTTON_R1           = (1u << 5),  // Right Bumper
    GAMEPAD_BUTTON_L3           = (1u << 6),  // Left Thumbstick Click
    GAMEPAD_BUTTON_R3           = (1u << 7),  // Right Thumbstick Click
    GAMEPAD_BUTTON_DPAD_UP      = (1u << 8),
    GAMEPAD_BUTTON_DPAD_DOWN    = (1u << 9),
    GAMEPAD_BUTTON_DPAD_LEFT    = (1u << 10),
    GAMEPAD_BUTTON_DPAD_RIGHT   = (1u << 11),
    GAMEPAD_BUTTON_START        = (1u << 12), // Options / Menu
    GAMEPAD_BUTTON_SELECT       = (1u << 13), // Share / Back
    GAMEPAD_BUTTON_HOME         = (1u << 14)  // System / Guide
};

typedef struct GamepadState {
    uint32_t buttons;       // Bitmask of active pressed buttons
    Vec2     leftStick;     // Filtered deadzone vector [-1.0, +1.0]
    Vec2     rightStick;    // Filtered deadzone vector [-1.0, +1.0]
    float    leftTrigger;   // Normalized trigger squeeze [0.0, 1.0]
    float    rightTrigger;  // Normalized trigger squeeze [0.0, 1.0]
    float    rumbleLowFreq; // Low frequency heavy rumble [0.0, 1.0]
    float    rumbleHighFreq;// High frequency sharp rumble [0.0, 1.0]
    bool     isConnected;
} GamepadState;

void GamepadState_init(GamepadState *state);
bool GamepadState_isDown(const GamepadState *state, uint32_t buttonMask);

// Circular deadzone math: dest-last order (writes to outFiltered)
void Gamepad_applyCircularDeadzone(Vec2 rawStick, float deadzone, Vec2 *outFiltered);

#endif
