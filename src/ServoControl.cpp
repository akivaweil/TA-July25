#include "ServoControl.h"

#define SERVO_FREQUENCY 50        // 50Hz for standard servos
#define SERVO_RESOLUTION 16       // 16-bit resolution
#define SERVO_PERIOD_US 20000     // 20ms period in microseconds

ServoControl::ServoControl() {
    minPulseWidth = 500;
    maxPulseWidth = 2500;
    minAngle = 0;
    maxAngle = 180;
    initialized = false;
    channel = -1;
}

void ServoControl::init(int servoPin) {
    this->servoPin = servoPin;

    // Find an available PWM channel (0-15)
    for (int i = 0; i < 16; i++) {
        if (!ledcRead(i)) {  // Check if channel is available
            channel = i;
            break;
        }
    }

    if (channel == -1) {
        // If no channel available, use channel 0 (might overwrite existing)
        channel = 0;
    }

    // Configure PWM
    ledcSetup(channel, SERVO_FREQUENCY, SERVO_RESOLUTION);
    ledcAttachPin(servoPin, channel);

    initialized = true;
}

void ServoControl::write(float angle) {
    if (!initialized) return;

    // Constrain angle to valid range
    if (angle < minAngle) angle = minAngle;
    if (angle > maxAngle) angle = maxAngle;

    // Convert angle to pulse width in microseconds
    int pulseWidth = map(angle, minAngle, maxAngle, minPulseWidth, maxPulseWidth);

    writeMicroseconds(pulseWidth);
}

void ServoControl::writeMicroseconds(int microseconds) {
    if (!initialized) return;

    // Constrain pulse width to valid range
    if (microseconds < minPulseWidth) microseconds = minPulseWidth;
    if (microseconds > maxPulseWidth) microseconds = maxPulseWidth;

    // Convert microseconds to duty cycle
    // Duty cycle = (pulse_width_us / period_us) * max_duty
    uint32_t maxDuty = (1 << SERVO_RESOLUTION) - 1;
    uint32_t duty = (microseconds * maxDuty) / SERVO_PERIOD_US;

    ledcWrite(channel, duty);
}

void ServoControl::detach() {
    if (initialized && channel >= 0) {
        ledcDetachPin(servoPin);
        initialized = false;
    }
}

void ServoControl::setPulseWidthRange(int minUs, int maxUs) {
    minPulseWidth = minUs;
    maxPulseWidth = maxUs;
}

void ServoControl::setAngleRange(int minDeg, int maxDeg) {
    minAngle = minDeg;
    maxAngle = maxDeg;
} 