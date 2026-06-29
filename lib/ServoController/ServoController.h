#pragma once

class ServoController {
public:
    void begin();
    void setTopServoAngle(int angle);
    void setLeftServoAngle(int angle);
    void setRightServoAngle(int angle);
};