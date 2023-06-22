#ifndef STEPPER_CLOCK_PWM_PENDULUM_H
#define STEPPER_CLOCK_PWM_PENDULUM_H

void pendulumSetEnabled(bool enabled);
void pendulumInit(double frequency, double highDuration, bool enabled);

#endif