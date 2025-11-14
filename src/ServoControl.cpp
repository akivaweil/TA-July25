#include "ServoControl.h"

ServoControl::ServoControl() {
    minPulseWidth = 500;
    maxPulseWidth = 2500;
    minAngle = 0;
    maxAngle = 180;
}

void ServoControl::init(int servoPin) {
    servo.attach(servoPin, minPulseWidth, maxPulseWidth);
}

void ServoControl::write(float angle) {
    // Constrain angle to valid range
    if (angle < minAngle) angle = minAngle;
    if (angle > maxAngle) angle = maxAngle;

    servo.write(angle);
}

void ServoControl::writeMicroseconds(int microseconds) {
    servo.writeMicroseconds(microseconds);
}

void ServoControl::detach() {
    servo.detach();
}

void ServoControl::setPulseWidthRange(int minUs, int maxUs) {
    minPulseWidth = minUs;
    maxPulseWidth = maxUs;
}

void ServoControl::setAngleRange(int minDeg, int maxDeg) {
    minAngle = minDeg;
    maxAngle = maxDeg;
} 