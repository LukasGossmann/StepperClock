#ifndef STEPPER_CLOCK_PWM_STEPPER_H
#define STEPPER_CLOCK_PWM_STEPPER_H

#include "pico/time.h"

void homeAndSeekClockHands();
void configurePwmForNormalOperation();
void stopNormalOperation();

#endif