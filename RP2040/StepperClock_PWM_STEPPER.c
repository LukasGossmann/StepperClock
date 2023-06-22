#include <math.h>

#include "StepperClock_Config.h"

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"

#include "StepperClock_PWM_STEPPER.h"

uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
    float x_float = (float)x;
    float in_min_float = (float)in_min;
    float in_max_float = (float)in_max;
    float out_min_float = (float)out_min;
    float out_max_float = (float)out_max;
    float result = (x_float - in_min_float) * (out_max_float - out_min_float) / (in_max_float - in_min_float) + out_min_float;
    return (uint32_t)roundf(result);
}

void convertTimeToSteps(int8_t hour, int8_t min, int8_t sec, uint32_t *stepsToNewHourPosition, uint32_t *stepsToNewMinutePosition)
{
    // Step counts are based on both hands being at 12 o'clock position
    // Each stepper has to move 51200 steps per revolution (200 fullsteps * 256 microsteps)
    const uint32_t stepsPerRevolution = 200 * 256;
    const uint32_t stepsPer5Minutes = stepsPerRevolution / 12;
    const uint32_t stepsPerSecond = stepsPerRevolution / 60 / 60;

    // Position for hour hands is calculated from current hour + and offset for the amout of minutes that already passed in that hour
    uint32_t hourLimitedTo12HourTime = hour % 12;
    *stepsToNewHourPosition = map(hourLimitedTo12HourTime, 0, 12, 0, stepsPerRevolution) + map(min, 0, 60, 0, stepsPer5Minutes);

    // Position for minute hands is calculated from current minute + an offset for the amount of seconds in that minute that already passed
    *stepsToNewMinutePosition = map(min, 0, 60, 0, stepsPerRevolution) + map(sec, 0, 60, 0, stepsPerSecond);
}

uint32_t configurePwmForHomeAndSeek(uint32_t pin, irq_handler_t pwmWrapIrqHandler)
{
    /// Setup gpio and pwm
    assert(pwm_gpio_to_channel(pin) == PWM_CHAN_A);

    gpio_set_outover(pin, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_NORMAL);

    gpio_init(pin);
    gpio_set_dir(pin, true);
    gpio_set_function(pin, GPIO_FUNC_PWM);
    const uint32_t slice_num = pwm_gpio_to_slice_num(pin);

    // Configure for 10khz
    pwm_config pwmConfig = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&pwmConfig, 1, 0);
    pwm_config_set_wrap(&pwmConfig, 12499);
    pwm_config_set_output_polarity(&pwmConfig, true, false);

    pwm_init(slice_num, &pwmConfig, false);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 12499 - 250);

    // Global irq setup
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwmWrapIrqHandler);

    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    return slice_num;
}

void deconfigurePwmFromHomeAndSeek(uint32_t pin, irq_handler_t pwmWrapIrqHandler)
{
    uint32_t slice_num = pwm_gpio_to_slice_num(pin);

    // Global irq setup
    pwm_set_enabled(slice_num, false);
    irq_set_enabled(PWM_IRQ_WRAP, false);
    pwm_set_irq_enabled(slice_num, false);

    // Only remove handler if one exists otherwise it might crash
    if (irq_get_exclusive_handler(PWM_IRQ_WRAP) != NULL)
        irq_remove_handler(PWM_IRQ_WRAP, pwmWrapIrqHandler);

    pwm_clear_irq(slice_num);
}

void configurePwmForNormalOperation()
{
    gpio_set_outover(MINUTE_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_NORMAL);
    gpio_set_outover(HOUR_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_NORMAL);

    /// Setup pwm for minute hand
    gpio_init(MINUTE_PULSE_PIN);
    gpio_set_dir(MINUTE_PULSE_PIN, true);
    gpio_set_function(MINUTE_PULSE_PIN, GPIO_FUNC_PWM);
    const uint32_t slice_num_minute = pwm_gpio_to_slice_num(MINUTE_PULSE_PIN);

    // Configure for 14.22222222222222222hz
    pwm_config pwmConfigMinute = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&pwmConfigMinute, 140, 10);
    pwm_config_set_wrap(&pwmConfigMinute, 62499);

    pwm_init(slice_num_minute, &pwmConfigMinute, false);
    pwm_set_chan_level(slice_num_minute, PWM_CHAN_A, 2);

    // Setup pwm for hour hand
    gpio_init(HOUR_PULSE_PIN);
    gpio_set_dir(HOUR_PULSE_PIN, true);
    gpio_set_function(HOUR_PULSE_PIN, GPIO_FUNC_PWM);
    const uint32_t slice_num_hour = pwm_gpio_to_slice_num(HOUR_PULSE_PIN);
    assert(pwm_gpio_to_channel(HOUR_PULSE_PIN) == PWM_CHAN_A);

    gpio_init(HOUR_PULSE_IN_PIN);
    gpio_set_dir(HOUR_PULSE_IN_PIN, false);
    gpio_set_function(HOUR_PULSE_IN_PIN, GPIO_FUNC_PWM);
    const uint32_t slice_num_hour_in_pin = pwm_gpio_to_slice_num(HOUR_PULSE_IN_PIN);
    assert(slice_num_hour == slice_num_hour_in_pin);
    assert(pwm_gpio_to_channel(HOUR_PULSE_IN_PIN) == PWM_CHAN_B);

    // Configure for counting up to 12
    pwm_config pwmConfigHour = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&pwmConfigHour, PWM_DIV_B_RISING);
    pwm_config_set_clkdiv_int_frac(&pwmConfigHour, 1, 0);
    pwm_config_set_wrap(&pwmConfigHour, 11);

    pwm_init(slice_num_hour, &pwmConfigHour, false);
    pwm_set_chan_level(slice_num_hour, PWM_CHAN_A, 1);

    // Mask enable so the pwm blocks run in sync
    const uint32_t hourMask = (1 << slice_num_hour);
    const uint32_t minuteMask = (1 << slice_num_minute);
    const uint32_t mask = hourMask | minuteMask;
    hw_set_bits(&pwm_hw->en, mask);
}

void stopNormalOperation()
{
    const uint32_t slice_num_minute = pwm_gpio_to_slice_num(MINUTE_PULSE_PIN);
    const uint32_t slice_num_hour = pwm_gpio_to_slice_num(HOUR_PULSE_PIN);
    const uint32_t hourMask = (1 << slice_num_hour);
    const uint32_t minuteMask = (1 << slice_num_minute);
    const uint32_t mask = hourMask | minuteMask;
    hw_clear_bits(&pwm_hw->en, mask);

    pwm_set_counter(slice_num_minute, 0);
    pwm_set_counter(slice_num_hour, 0);

    // Force pins low as state after disable depends on the last value in the counter
    gpio_set_outover(MINUTE_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_LOW);
    gpio_set_outover(HOUR_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_LOW);
}

// TODO: There can still be edge cases where the dial is just barely triggering the limit switch
bool isFirstIrq = true;
bool minuteHomeSwitchCleared = false;
bool hourHomeSwitchCleared = false;
bool minuteHomeSwitchStateOnFirstIrq = false;
bool hourHomeSwitchStateOnFirstIrq = false;
void pwm_wrap_irq_handler_home()
{
    const uint32_t allGpioStates = gpio_get_all();
    const bool minuteHomePin = (allGpioStates & (1 << MINUTE_HOME_PIN)) >> MINUTE_HOME_PIN;
    const bool hourHomePin = (allGpioStates & (1 << HOUR_HOME_PIN)) >> HOUR_HOME_PIN;

    if (isFirstIrq)
    {
        isFirstIrq = false;
        minuteHomeSwitchStateOnFirstIrq = minuteHomePin;
        hourHomeSwitchStateOnFirstIrq = hourHomePin;
    }

    // If the switch was already cleared before starting there is nothing to detect otheriwise detect the
    // transition from switch being triggered to switch being untriggered (false = triggered, true = untriggered)
    if (minuteHomeSwitchStateOnFirstIrq || (!minuteHomeSwitchStateOnFirstIrq && !minuteHomeSwitchCleared && minuteHomePin))
        minuteHomeSwitchCleared = true;

    if (hourHomeSwitchStateOnFirstIrq || (!hourHomeSwitchStateOnFirstIrq && !hourHomeSwitchCleared && hourHomePin))
        hourHomeSwitchCleared = true;

    const uint32_t slice_num_minute = pwm_gpio_to_slice_num(MINUTE_PULSE_PIN);
    const uint32_t slice_num_hour = pwm_gpio_to_slice_num(HOUR_PULSE_PIN);

    // Only once the homing switches have been cleared the normal homing procedure can begin
    if (minuteHomeSwitchCleared && !minuteHomePin)
        pwm_set_enabled(slice_num_minute, false);

    if (hourHomeSwitchCleared && !hourHomePin)
        pwm_set_enabled(slice_num_hour, false);

    // The same as but exactly at the same time
    // pwm_clear_irq(slice_num_minute);
    // pwm_clear_irq(slice_num_hour);
    pwm_hw->intr = (1u << slice_num_minute) | (1u << slice_num_hour);
}

void homeClockHands()
{
    // Deinit (high impedance) hour pulse in pin as it is connected to theminute pulse output pin
    gpio_deinit(HOUR_PULSE_IN_PIN);

    const uint32_t hourSliceNum = configurePwmForHomeAndSeek(HOUR_PULSE_PIN, &pwm_wrap_irq_handler_home);
    const uint32_t minuteSliceNum = configurePwmForHomeAndSeek(MINUTE_PULSE_PIN, &pwm_wrap_irq_handler_home);

    // State variables for "unhoming" before homing again
    isFirstIrq = true;
    minuteHomeSwitchCleared = false;
    hourHomeSwitchCleared = false;
    minuteHomeSwitchStateOnFirstIrq = false;
    hourHomeSwitchStateOnFirstIrq = false;

    // Init gpio pins for homing switches
    gpio_init(HOUR_HOME_PIN);
    gpio_set_dir(HOUR_HOME_PIN, false);

    gpio_init(HOUR_HOME_POWER_PIN);
    gpio_set_dir(HOUR_HOME_POWER_PIN, true);
    gpio_put(HOUR_HOME_POWER_PIN, true);

    gpio_init(MINUTE_HOME_PIN);
    gpio_set_dir(MINUTE_HOME_PIN, false);

    gpio_init(MINUTE_HOME_POWER_PIN);
    gpio_set_dir(MINUTE_HOME_POWER_PIN, true);
    gpio_put(MINUTE_HOME_POWER_PIN, true);

    // Mask enable so the pwm blocks run in sync
    const uint32_t hourMask = (1 << hourSliceNum);
    const uint32_t minuteMask = (1 << minuteSliceNum);
    const uint32_t mask = hourMask | minuteMask;
    hw_set_bits(&pwm_hw->en, mask);

    // Wait until both pwm units got disabled by the homing irq handler
    while ((pwm_hw->en & mask) != 0)
        tight_loop_contents();

    gpio_put(HOUR_HOME_POWER_PIN, false);
    gpio_put(MINUTE_HOME_POWER_PIN, false);

    deconfigurePwmFromHomeAndSeek(HOUR_PULSE_PIN, &pwm_wrap_irq_handler_home);
    deconfigurePwmFromHomeAndSeek(MINUTE_PULSE_PIN, &pwm_wrap_irq_handler_home);
}

volatile uint32_t hourStepCount = 0;
volatile uint32_t minuteStepCount = 0;

void pwm_wrap_irq_handler_seek()
{
    const uint32_t slice_num_minute = pwm_gpio_to_slice_num(MINUTE_PULSE_PIN);

    if (--minuteStepCount == 0)
        pwm_set_enabled(slice_num_minute, false);

    const uint32_t slice_num_hour = pwm_gpio_to_slice_num(HOUR_PULSE_PIN);

    if (--hourStepCount == 0)
        pwm_set_enabled(slice_num_hour, false);

    pwm_clear_irq(slice_num_minute);
    pwm_clear_irq(slice_num_hour);
}

void seekClockHands()
{
    // Deinit (high impedance) hour pulse in pin as it is connected to theminute pulse output pin
    gpio_deinit(HOUR_PULSE_IN_PIN);

    datetime_t dateTime;
    rtc_get_datetime(&dateTime);

    // Step count from 12 o'clock position to given time
    uint32_t tmpHourStepCount;
    uint32_t tmpMinuteStepCount;
    convertTimeToSteps(dateTime.hour, dateTime.min, dateTime.sec, &tmpHourStepCount, &tmpMinuteStepCount);

    // Step counts are based on both hands being at 12 o'clock position
    // Each stepper has to move 51200 steps per revolution (200 fullsteps * 256 microsteps)
    const uint32_t stepsPerRevolution = 200 * 256;
    const uint32_t secondsPerHour = 60 * 60 * 1;

    uint32_t hourHandOffset = map(HOME_POSITION_OFFSET_HOUR, 0, secondsPerHour, 0, stepsPerRevolution);
    uint32_t minuteHandOffset = map(HOME_POSITION_OFFSET_MINUTE, 0, secondsPerHour, 0, stepsPerRevolution);

    hourStepCount = tmpHourStepCount + hourHandOffset;
    minuteStepCount = tmpMinuteStepCount + minuteHandOffset;

    // printf("Steps to current time: hour: %i, minute: %i\n", hourStepCount, minuteStepCount);

    if ((hourStepCount > 0) || (minuteStepCount > 0))
    {
        const uint32_t hourSliceNum = configurePwmForHomeAndSeek(HOUR_PULSE_PIN, &pwm_wrap_irq_handler_seek);
        const uint32_t minuteSliceNum = configurePwmForHomeAndSeek(MINUTE_PULSE_PIN, &pwm_wrap_irq_handler_seek);

        // Mask enable so the pwm blocks run in sync
        // Do not enable the pwm if the minute or hour hand are at the correct position already (12 o'clock, 0 steps to correct position)
        const uint32_t hourMask = hourStepCount > 0 ? (1 << hourSliceNum) : 0;
        const uint32_t minuteMask = minuteStepCount > 0 ? (1 << minuteSliceNum) : 0;
        const uint32_t mask = hourMask | minuteMask;
        hw_set_bits(&pwm_hw->en, mask);

        // Wait until both pwm units got disabled by the homing irq handler
        while ((pwm_hw->en & mask) != 0)
            tight_loop_contents();

        deconfigurePwmFromHomeAndSeek(HOUR_PULSE_PIN, &pwm_wrap_irq_handler_seek);
        deconfigurePwmFromHomeAndSeek(MINUTE_PULSE_PIN, &pwm_wrap_irq_handler_seek);
    }
}

void homeAndSeekClockHands()
{
    homeClockHands();
    seekClockHands();
}
