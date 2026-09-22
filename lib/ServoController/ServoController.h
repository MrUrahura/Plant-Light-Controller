#pragma once
#include <ESP32Servo.h>
#include <Preferences.h>

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
    void update();
    bool isMoving() const;

private:
    Servo topServo;
    Servo leftServo;
    Servo rightServo;
    uint8_t shadeState;
    Preferences prefs;

    // Eventually, instead of hardcoding the times, use the app to calibrate them
    // Or implement an encoder or limit switches to control distance
    static constexpr uint8_t SERVO_STOP = 90;
    static constexpr uint8_t FORWARD_SPEED = 0;
    static constexpr uint8_t REVERSE_SPEED = 180;

    static constexpr uint32_t TOP_OPEN_TIME_MS = 2000;
    static constexpr uint32_t TOP_CLOSE_TIME_MS = 2000;

    static constexpr uint32_t LEFT_OPEN_TIME_MS = 5000;
    static constexpr uint32_t LEFT_CLOSE_TIME_MS = 5000;

    static constexpr uint32_t RIGHT_OPEN_TIME_MS = 5000;
    static constexpr uint32_t RIGHT_CLOSE_TIME_MS = 5000;

    bool topMoving;
    bool leftMoving;
    bool rightMoving;
    uint32_t topDuration = 0;
    uint32_t leftDuration = 0;
    uint32_t rightDuration = 0;

    uint32_t topStartTime = 0;
    uint32_t leftStartTime = 0;
    uint32_t rightStartTime = 0;

    void moveServo(Servo& servo, uint8_t speed, uint32_t timeMs);
    void saveState();
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