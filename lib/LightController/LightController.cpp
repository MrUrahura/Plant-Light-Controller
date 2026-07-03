#include <Arduino.h>
#include "LightController.h"

// Enum to represent the state of the light control system
enum class ControlState {
    MAXIMIZE_LIGHT,
    OPTIMIZE_LIGHT,
    LIMIT_LIGHT,
    PHOTOPERIOD_COMPLETE
};
ControlState currentControlState = ControlState::MAXIMIZE_LIGHT;