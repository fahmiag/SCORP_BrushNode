#pragma once
#include <Arduino.h>
namespace Config {
	constexpr uint8_t motorPwm = 9, motorDir = 8;
	constexpr uint8_t axisPwm = 10, axisDir = 11;
	constexpr uint8_t topLimit = A0, bottomLimit = A1;
	constexpr uint8_t forwardLevel = HIGH, upLevel = HIGH;
	constexpr unsigned long baud = 115200;
	constexpr unsigned long communicationTimeoutMs = 3000;
	constexpr unsigned long travelTimeoutMs = 120000;
	constexpr unsigned long topWaitMs = 2000;
	constexpr unsigned long rampMs = 1000; // 0 to full PWM
	constexpr unsigned long reversalPauseMs = 100;
	constexpr uint8_t brushSpeed = 255, axisSpeed = 255;
}

