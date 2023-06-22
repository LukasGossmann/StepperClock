#include "hardware/rtc.h"
#include "hardware/gpio.h"
// For scb_hw so we can disable deep sleep
#include "hardware/structs/scb.h"

#include "StepperClock_Config.h"
#include "StepperClock_RTC.h"
#include "StepperClock_PWM_PENDULUM.h"
#include "StepperClock_PWM_STEPPER.h"

#include <stdio.h>

volatile bool CLOCK_RESTART_TRIGGER = false;

void clock_sleep_mode()
{
    pendulumSetEnabled(false);
    stopNormalOperation();

    // Wait 500ms before power down. Needs to use busy wait since sleep_ms does not work in an IRQ
    busy_wait_us(500000);
    gpio_put(TMC_ENABLE_PIN, true);
}

void clock_wake_up()
{
    pendulumSetEnabled(true);
    gpio_put(TMC_ENABLE_PIN, false);

    // This will make the main loop call the homing function since handling another IRQ is not possible while already in an IRQ
    CLOCK_RESTART_TRIGGER = true;

    // Disable sleep on exit of IRQ hanlder
    scb_hw->scr &= ~M0PLUS_SCR_SLEEPONEXIT_BITS;
}

void rtc_callback()
{
    rtc_disable_alarm();

    datetime_t currentDateTime;
    rtc_get_datetime(&currentDateTime);

    datetime_t dateTime;
    dateTime.year = -1;
    dateTime.month = -1;
    dateTime.day = -1;
    dateTime.dotw = -1;
    dateTime.hour = -1;
    dateTime.min = 0;
    dateTime.sec = 0;

    if (currentDateTime.hour == CLOCK_SLEEP_MODE_HOUR)
    {
        dateTime.hour = CLOCK_WAKE_UP_HOUR;
        clock_sleep_mode();
    }
    else if (currentDateTime.hour == CLOCK_WAKE_UP_HOUR)
    {
        dateTime.hour = CLOCK_SLEEP_MODE_HOUR;
        clock_wake_up();
    }

    rtc_set_alarm(&dateTime, rtc_callback);
    rtc_enable_alarm();
}

void rtcInit(datetime_t *t)
{
    rtc_init();
    rtc_set_datetime(t);

    datetime_t dateTime;
    dateTime.year = -1;
    dateTime.month = -1;
    dateTime.day = -1;
    dateTime.dotw = -1;
    dateTime.hour = -1;
    dateTime.min = 0;
    dateTime.sec = 0;

    if (t->hour >= CLOCK_WAKE_UP_HOUR && t->hour < CLOCK_SLEEP_MODE_HOUR)
    {
        dateTime.hour = CLOCK_SLEEP_MODE_HOUR;
    }
    else if (t->hour < CLOCK_WAKE_UP_HOUR && t->hour >= CLOCK_SLEEP_MODE_HOUR)
    {
        dateTime.hour = CLOCK_WAKE_UP_HOUR;
    }

    rtc_set_alarm(&dateTime, rtc_callback);
    rtc_enable_alarm();
}
