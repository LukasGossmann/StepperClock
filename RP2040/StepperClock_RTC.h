#ifndef STEPPER_CLOCK_RTC_H
#define STEPPER_CLOCK_RTC_H

#include "pico/types.h"

void rtcInit(datetime_t *t);
extern volatile bool CLOCK_RESTART_TRIGGER;

#endif