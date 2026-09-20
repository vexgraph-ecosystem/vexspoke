#include "input/gamepad.h"

#include <math.h>
#include <string.h>
#include "annotation/definition.h"
#include "annotation/overview.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Gamepad
 * ============================================================================
 * Processes digital game controller buttons, dual-motor rumble haptics, and
 * radial circular deadzone vector mapping for analog thumbsticks. GamepadState
 * is a plain value struct (no heap allocation) holding the button bitmask,
 * filtered stick vectors, trigger squeezes, rumble intensities, and the
 * connection flag; callers own it on the stack or in a device slot. The
 * deadzone filter rescales stick vectors from (deadzone..1.0) to (0.0..1.0)
 * with dest-last output, keeping the analog model consistent across platforms.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Gamepad (input/gamepad.c)
 * LEVEL: L2 — Game Controller Hardware Input
 * ============================================================================
 * Processes digital game controller buttons, dual-motor rumble haptics,
 * and radial circular deadzone vector mapping for analog thumbsticks.
 *
 * STRUCT FIELDS (Mirroring input/gamepad.h):
 * ----------------------------------------------------------------------------
 *   GamepadState {
 *     uint32_t buttons;       // bitmask of active pressed buttons
 *     Vec2     leftStick;     // filtered deadzone vector [-1.0, +1.0]
 *     Vec2     rightStick;    // filtered deadzone vector [-1.0, +1.0]
 *     float    leftTrigger;   // normalized trigger squeeze [0.0, 1.0]
 *     float    rightTrigger;  // normalized trigger squeeze [0.0, 1.0]
 *     float    rumbleLowFreq; // low frequency heavy rumble [0.0, 1.0]
 *     float    rumbleHighFreq;// high frequency sharp rumble [0.0, 1.0]
 *     bool     isConnected;
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - GamepadState_init(state)
 *   - GamepadState_isDown(state, buttonMask)
 *   - Gamepad_applyCircularDeadzone(rawStick, deadzone, outFiltered)
 * ============================================================================
 */

void GamepadState_init(GamepadState *state) {
    if (state == nullptr) return;
    memset(state, 0, sizeof(GamepadState));
    (*state).isConnected = false;
}

bool GamepadState_isDown(const GamepadState *state, uint32_t buttonMask) {
    if (state == nullptr) return false;
    return ((*state).buttons & buttonMask) == buttonMask;
}

void Gamepad_applyCircularDeadzone(Vec2 rawStick, float deadzone, Vec2 *outFiltered) {
    if (outFiltered == nullptr) return;

    if (deadzone <= 0.0f) {
        *outFiltered = rawStick;
        return;
    }
    if (deadzone >= 0.99f) {
        deadzone = 0.99f;
    }

    float mag = sqrtf(rawStick.horizontal * rawStick.horizontal +
                      rawStick.vertical   * rawStick.vertical);

    if (mag <= deadzone) {
        (*outFiltered).horizontal = 0.0f;
        (*outFiltered).vertical   = 0.0f;
        return;
    }

    // Rescale vector smoothly from (deadzone..1.0) to (0.0..1.0)
    float normalizedMag = (mag - deadzone) / (1.0f - deadzone);
    if (normalizedMag > 1.0f) {
        normalizedMag = 1.0f;
    }

    float scale = normalizedMag / mag;
    (*outFiltered).horizontal = rawStick.horizontal * scale;
    (*outFiltered).vertical   = rawStick.vertical   * scale;
}
