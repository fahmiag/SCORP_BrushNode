#pragma once
#include <Arduino.h>
enum class Axis { Idle, Up, Wait, Down };
void beginStateMachine();
void updateStateMachine(unsigned long now);
void stopAxis();
void stopAll();
void trip(const char *reason);
bool bothLimits();
bool topActive();
bool bottomActive();
Axis axisState();
bool hasFault();
void clearFault();
bool startCycle(unsigned long now);
