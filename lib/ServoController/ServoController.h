#pragma once
#include <ESP32Servo.h>

class ServoController {
public:
    ServoController();
    void begin();

    // Enum to represent the state of the shades
    enum class ShadeID : uint8_t {
        NONE = 0,
        TOP_SHADE = 1 << 0,
        LEFT_SHADE = 1 << 1,
        RIGHT_SHADE = 1 << 2,
        ALL = 0x07
    };
    uint8_t getCurrentState();
    bool areShadesClosed(ShadeID shades) const;
    void setShades(ShadeID shades);
    void setShades(uint8_t shades);

private:
    Servo topServo;
    Servo leftServo;
    Servo rightServo;
    uint8_t shadeState;
};

// Enable bitwise operators for our enum class so the code looks clean
inline ServoController::ShadeID operator|(ServoController::ShadeID a, ServoController::ShadeID b) {
    return static_cast<ServoController::ShadeID>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
// Allows bitwise AND between two ShadeID enums, returning an integer (or bool check)
inline uint8_t operator&(ServoController::ShadeID a, ServoController::ShadeID b) {
    return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}
// Allows bitwise AND between a uint8_t and a ShadeID enum, returning an integer (or bool check)
inline uint8_t operator&(uint8_t a, ServoController::ShadeID b) {
    return a & static_cast<uint8_t>(b);
}