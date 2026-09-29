#include <Arduino.h>
#include "ServoController.h"
#include "Pins.h"

ServoController::ServoController()
    : shadeState(static_cast<uint8_t>(ShadeID::ALL)),
    targetShadeState(static_cast<uint8_t>(ShadeID::ALL)),
    hasPendingState(false),
    topMoving(false),
    leftMoving(false),
    rightMoving(false)
{
}

void ServoController::begin() {
    // Configure the PWM channels at startup, then release PWM before accepting commands.
    attachServos();
    detachServos();

    topMoving = false;
    leftMoving = false;
    rightMoving = false;

    // Calibration Code: assume all shades begin closed due to manual intervention.
    // shadeState = static_cast<uint8_t>(ShadeID::ALL);

    // Regular Operation Code
    prefs.begin("servos", true);
    shadeState = prefs.getUChar("shadeState", static_cast<uint8_t>(ShadeID::ALL));
    topOpenTimeMs = prefs.getUInt("topOpenMs", DEFAULT_TOP_OPEN_TIME_MS);
    topCloseTimeMs = prefs.getUInt("topCloseMs", DEFAULT_TOP_CLOSE_TIME_MS);
    leftOpenTimeMs = prefs.getUInt("leftOpenMs", DEFAULT_LEFT_OPEN_TIME_MS);
    leftCloseTimeMs = prefs.getUInt("leftCloseMs", DEFAULT_LEFT_CLOSE_TIME_MS);
    rightOpenTimeMs = prefs.getUInt("rightOpenMs", DEFAULT_RIGHT_OPEN_TIME_MS);
    rightCloseTimeMs = prefs.getUInt("rightCloseMs", DEFAULT_RIGHT_CLOSE_TIME_MS);
    prefs.end();

    if (topOpenTimeMs == 0 || topOpenTimeMs > 60000) topOpenTimeMs = DEFAULT_TOP_OPEN_TIME_MS;
    if (topCloseTimeMs == 0 || topCloseTimeMs > 60000) topCloseTimeMs = DEFAULT_TOP_CLOSE_TIME_MS;
    if (leftOpenTimeMs == 0 || leftOpenTimeMs > 60000) leftOpenTimeMs = DEFAULT_LEFT_OPEN_TIME_MS;
    if (leftCloseTimeMs == 0 || leftCloseTimeMs > 60000) leftCloseTimeMs = DEFAULT_LEFT_CLOSE_TIME_MS;
    if (rightOpenTimeMs == 0 || rightOpenTimeMs > 60000) rightOpenTimeMs = DEFAULT_RIGHT_OPEN_TIME_MS;
    if (rightCloseTimeMs == 0 || rightCloseTimeMs > 60000) rightCloseTimeMs = DEFAULT_RIGHT_CLOSE_TIME_MS;
    
    targetShadeState = shadeState;
    hasPendingState = false;
    saveOnComplete = false;
}

uint8_t ServoController::getCurrentState() {
    return shadeState;
}

bool ServoController::areShadesClosed(ShadeID shades) const {
    return (shadeState & shades) != 0;
}

bool ServoController::setShades(ShadeID shades, bool saveToPrefs) {
    return setShades(static_cast<uint8_t>(shades), saveToPrefs);
}

bool ServoController::setShades(uint8_t shades, bool saveToPrefs) {
    constexpr uint8_t VALID_SHADE_MASK = static_cast<uint8_t>(ShadeID::ALL);

    Serial.print("SERVO COMMAND: current=");
    Serial.print(shadeState);
    Serial.print(" target=");
    Serial.println(static_cast<uint8_t>(shades));

    if ((shades & ~VALID_SHADE_MASK) != 0) {
        Serial.println("ServoController: Command rejected - invalid shade state.");
        return false;
    }
    
    // A new movement cannot interrupt an existing movement.
    if (isMoving()) {
        Serial.println("ServoController: Command rejected - servos are still moving.");
        return false;
    }

    // Already at requested state.
    if (shades == shadeState) {
        Serial.println("ServoController: Already at requested state.");
        detachServos();
        return true;
    }

    attachServos();
    saveOnComplete = saveToPrefs;
    
    targetShadeState = shades;
    hasPendingState = true;

    // Top
    if ((shadeState & static_cast<uint8_t>(ShadeID::TOP_SHADE)) !=
        (shades & static_cast<uint8_t>(ShadeID::TOP_SHADE)))
    {
        if (shades & static_cast<uint8_t>(ShadeID::TOP_SHADE))
            moveServo(topServo, FORWARD_SPEED, topCloseTimeMs);
        else
            moveServo(topServo, REVERSE_SPEED, topOpenTimeMs);
    }

    // Left
    if ((shadeState & static_cast<uint8_t>(ShadeID::LEFT_SHADE)) !=
        (shades & static_cast<uint8_t>(ShadeID::LEFT_SHADE)))
    {
        if (shades & static_cast<uint8_t>(ShadeID::LEFT_SHADE))
            moveServo(leftServo, FORWARD_SPEED, leftCloseTimeMs);
        else
            moveServo(leftServo, REVERSE_SPEED, leftOpenTimeMs);
    }

    // Right
    if ((shadeState & static_cast<uint8_t>(ShadeID::RIGHT_SHADE)) !=
        (shades & static_cast<uint8_t>(ShadeID::RIGHT_SHADE)))
    {
        if (shades & static_cast<uint8_t>(ShadeID::RIGHT_SHADE))
            moveServo(rightServo, REVERSE_SPEED, rightCloseTimeMs);
        else
            moveServo(rightServo, FORWARD_SPEED, rightOpenTimeMs);
    }

    return true;
}

bool ServoController::setMovementTimes(
    uint32_t newTopOpenMs,
    uint32_t newTopCloseMs,
    uint32_t newLeftOpenMs,
    uint32_t newLeftCloseMs,
    uint32_t newRightOpenMs,
    uint32_t newRightCloseMs
) {
    constexpr uint32_t MIN_MOVEMENT_TIME_MS = 1;
    constexpr uint32_t MAX_MOVEMENT_TIME_MS = 60000;
    const uint32_t times[] = {
        newTopOpenMs, newTopCloseMs, newLeftOpenMs,
        newLeftCloseMs, newRightOpenMs, newRightCloseMs
    };

    for (uint32_t duration : times) {
        if (duration < MIN_MOVEMENT_TIME_MS || duration > MAX_MOVEMENT_TIME_MS) {
            Serial.println("ServoController: Movement times must be between 1 and 60000 ms.");
            return false;
        }
    }

    if (isMoving()) {
        Serial.println("ServoController: Movement times cannot change while servos are moving.");
        return false;
    }

    topOpenTimeMs = newTopOpenMs;
    topCloseTimeMs = newTopCloseMs;
    leftOpenTimeMs = newLeftOpenMs;
    leftCloseTimeMs = newLeftCloseMs;
    rightOpenTimeMs = newRightOpenMs;
    rightCloseTimeMs = newRightCloseMs;

    prefs.begin("servos", false);
    prefs.putUInt("topOpenMs", topOpenTimeMs);
    prefs.putUInt("topCloseMs", topCloseTimeMs);
    prefs.putUInt("leftOpenMs", leftOpenTimeMs);
    prefs.putUInt("leftCloseMs", leftCloseTimeMs);
    prefs.putUInt("rightOpenMs", rightOpenTimeMs);
    prefs.putUInt("rightCloseMs", rightCloseTimeMs);
    prefs.end();
    return true;
}

uint32_t ServoController::getTopOpenTimeMs() const { return topOpenTimeMs; }
uint32_t ServoController::getTopCloseTimeMs() const { return topCloseTimeMs; }
uint32_t ServoController::getLeftOpenTimeMs() const { return leftOpenTimeMs; }
uint32_t ServoController::getLeftCloseTimeMs() const { return leftCloseTimeMs; }
uint32_t ServoController::getRightOpenTimeMs() const { return rightOpenTimeMs; }
uint32_t ServoController::getRightCloseTimeMs() const { return rightCloseTimeMs; }

void ServoController::moveServo(Servo& servo, uint8_t speed, uint32_t timeMs) {
    Serial.print("MOVING: ");
    if (&servo == &topServo){
        Serial.print("TOP");
    } else if (&servo == &leftServo) {
        Serial.print("LEFT");
    } else if (&servo == &rightServo) {
        Serial.print("RIGHT");
    }
    Serial.print(" speed=");
    Serial.print(speed);
    Serial.print(" duration=");
    Serial.println(timeMs);

    servo.write(speed);

    if (&servo == &topServo) {
        topMoving = true;
        topStartTime = millis();
        topDuration = timeMs;
    }
    else if (&servo == &leftServo) {
        leftMoving = true;
        leftStartTime = millis();
        leftDuration = timeMs;
    }
    else if (&servo == &rightServo) {
        rightMoving = true;
        rightStartTime = millis();
        rightDuration = timeMs;
    }
}

bool ServoController::isMoving() const {
    return topMoving || leftMoving || rightMoving;
}

void ServoController::update() {
    uint32_t now = millis();

    if (topMoving && (now - topStartTime >= topDuration))
    {
        topServo.release();
        topMoving = false;
        Serial.println("Stopping top servo...");
    }

    if (leftMoving && (now - leftStartTime >= leftDuration))
    {
        leftServo.release();
        leftMoving = false;
        Serial.println("Stopping left servo...");
    }

    if (rightMoving && (now - rightStartTime >= rightDuration))
    {
        rightServo.release();
        rightMoving = false;
        Serial.println("Stopping right servo...");
    }

    // Only now, after every servo has actually finished, does the controller commit the new state.
    if (hasPendingState && !isMoving())
    {
        shadeState = targetShadeState;
        hasPendingState = false;

        if (saveOnComplete) {
            digitalWrite(LED_BUILTIN, HIGH);
            saveState();
            Serial.println("ServoController: State saved to NVS.");
            saveOnComplete = false;
        } else {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("ServoController: Scan step complete (NVS save skipped).");
        }

        detachServos();
        Serial.print("ServoController: Movement complete. State = ");
        Serial.println(shadeState);
    }
}

void ServoController::saveState() {
    prefs.begin("servos", false);
    prefs.putUChar("shadeState", shadeState);
    prefs.end();
}

void ServoController::attachServos() {
    if (!topServo.attached()) {
        topServo.attach(TOP_SERVO_PIN);
    }
    if (!leftServo.attached()) {
        leftServo.attach(LEFT_SERVO_PIN);
    }
    if (!rightServo.attached()) {
        rightServo.attach(RIGHT_SERVO_PIN);
    }
}

void ServoController::detachServos() {
    if (topServo.attached()) {
        topServo.release();
        topServo.detach();
    }
    if (leftServo.attached()) {
        leftServo.release();
        leftServo.detach();
    }
    if (rightServo.attached()) {
        rightServo.release();
        rightServo.detach();
    }
}