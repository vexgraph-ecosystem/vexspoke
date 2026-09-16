#include "input/gamepad.h"

#include <math.h>
#include <string.h>
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Gamepad (input/gamepad.c)
 * LEVEL: L2 — Game Controller Hardware Input
 * ============================================================================
 * Processes digital game controller buttons, dual-motor rumble haptics,
 * and radial circular deadzone vector mapping for analog thumbsticks.
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
