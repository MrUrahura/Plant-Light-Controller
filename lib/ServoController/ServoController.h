#pragma once
#include <ESP32Servo.h>
#include <Preferences.h>

class ServoController {
public:
    ServoController();
    void begin();

    enum class ShadeID : uint8_t {
        NONE = 0,
        TOP_SHADE = 1 << 0,
        LEFT_SHADE = 1 << 1,
        RIGHT_SHADE = 1 << 2,
        ALL = 0x07
    };

    uint8_t getCurrentState();
    bool areShadesClosed(ShadeID shades) const;
    bool setShades(ShadeID shades, bool saveToPrefs);
    bool setShades(uint8_t shades, bool saveToPrefs);
    bool setMovementTimes(
        uint32_t topOpenMs,
        uint32_t topCloseMs,
        uint32_t leftOpenMs,
        uint32_t leftCloseMs,
        uint32_t rightOpenMs,
        uint32_t rightCloseMs
    );
    uint32_t getTopOpenTimeMs() const;
    uint32_t getTopCloseTimeMs() const;
    uint32_t getLeftOpenTimeMs() const;
    uint32_t getLeftCloseTimeMs() const;
    uint32_t getRightOpenTimeMs() const;
    uint32_t getRightCloseTimeMs() const;
    void update();
    bool isMoving() const;

private:
    Servo topServo;
    Servo leftServo;
    Servo rightServo;
    Preferences prefs;

    // This represents the last physically completed shade state.
    uint8_t shadeState;

    // State currently being physically moved toward.
    uint8_t targetShadeState;
    bool hasPendingState;

    static constexpr uint8_t FORWARD_SPEED = 0;
    static constexpr uint8_t REVERSE_SPEED = 180;

    static constexpr uint32_t DEFAULT_TOP_OPEN_TIME_MS = 3825;
    static constexpr uint32_t DEFAULT_TOP_CLOSE_TIME_MS = 3450;

    static constexpr uint32_t DEFAULT_LEFT_OPEN_TIME_MS = 5000;
    static constexpr uint32_t DEFAULT_LEFT_CLOSE_TIME_MS = 4800;

    static constexpr uint32_t DEFAULT_RIGHT_OPEN_TIME_MS = 4625;
    static constexpr uint32_t DEFAULT_RIGHT_CLOSE_TIME_MS = 4350;

    uint32_t topOpenTimeMs = DEFAULT_TOP_OPEN_TIME_MS;
    uint32_t topCloseTimeMs = DEFAULT_TOP_CLOSE_TIME_MS;

    uint32_t leftOpenTimeMs = DEFAULT_LEFT_OPEN_TIME_MS;
    uint32_t leftCloseTimeMs = DEFAULT_LEFT_CLOSE_TIME_MS;

    uint32_t rightOpenTimeMs = DEFAULT_RIGHT_OPEN_TIME_MS;
    uint32_t rightCloseTimeMs = DEFAULT_RIGHT_CLOSE_TIME_MS;

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
    bool saveOnComplete = false;
    void saveState();
    void attachServos();
    void detachServos();
};

inline ServoController::ShadeID operator|(
    ServoController::ShadeID a,
    ServoController::ShadeID b
) {
    return static_cast<ServoController::ShadeID>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

inline uint8_t operator&(
    ServoController::ShadeID a,
    ServoController::ShadeID b
) {
    return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

inline uint8_t operator&(
    uint8_t a,
    ServoController::ShadeID b
) {
    return a & static_cast<uint8_t>(b);
}