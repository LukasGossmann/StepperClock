#include <math.h>

#include "StepperClock_Config.h"
#include "StepperClock_PWM_PENDULUM.h"

#include "hardware/pwm.h"
#include "hardware/gpio.h"

void pendulumSetEnabled(bool enabled)
{
    const uint32_t slice_num_pendulum = pwm_gpio_to_slice_num(PENDULUM_PULSE_PIN);
    const uint32_t slice_num_pendulum_base = pwm_gpio_to_slice_num(PENDULUM_BASE_PULSE_PIN);

    if (enabled)
    {
        gpio_set_outover(PENDULUM_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_NORMAL);
        gpio_set_outover(PENDULUM_BASE_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_NORMAL);
    }
    else
    {
        pwm_set_counter(slice_num_pendulum, 0);
        pwm_set_counter(slice_num_pendulum_base, 0);

        // Force pins low as state after disable depends on the last value in the counter
        gpio_set_outover(PENDULUM_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_LOW);
        gpio_set_outover(PENDULUM_BASE_PULSE_PIN, IO_BANK0_GPIO0_CTRL_OUTOVER_VALUE_LOW);
    }

    // Enable dividing pwm unit first and clock generator pwm unit second
    pwm_set_enabled(slice_num_pendulum, enabled);
    pwm_set_enabled(slice_num_pendulum_base, enabled);
}

/// @brief Generates a signal with the given frequency and high duration
/// @param frequency The frequency in Hz with a resolution of 0.0001Hz
/// @param highDuration The high duration in ms with a resolution of 0.1ms (100us)
/// @param enabled True to enable the pendulum after init is done
void pendulumInit(double frequency, double highDuration, bool enabled)
{
    /// Setup pwm base
    gpio_init(PENDULUM_BASE_PULSE_PIN);
    gpio_set_dir(PENDULUM_BASE_PULSE_PIN, true);
    gpio_set_function(PENDULUM_BASE_PULSE_PIN, GPIO_FUNC_PWM);
    const uint32_t slice_num_pendulum_base = pwm_gpio_to_slice_num(PENDULUM_BASE_PULSE_PIN);

    // Configure for 1khz
    pwm_config pwmConfigPendulumBase = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&pwmConfigPendulumBase, 1, 0);
    pwm_config_set_wrap(&pwmConfigPendulumBase, 12499);

    pwm_init(slice_num_pendulum_base, &pwmConfigPendulumBase, false);
    pwm_set_chan_level(slice_num_pendulum_base, PWM_CHAN_A, 6250);

    // Setup pwm for division
    gpio_init(PENDULUM_PULSE_PIN);
    gpio_set_dir(PENDULUM_PULSE_PIN, true);
    gpio_set_function(PENDULUM_PULSE_PIN, GPIO_FUNC_PWM);
    const uint32_t slice_num_pendulum = pwm_gpio_to_slice_num(PENDULUM_PULSE_PIN);
    assert(pwm_gpio_to_channel(PENDULUM_PULSE_PIN) == PWM_CHAN_A);

    gpio_init(PENDULUM_PULSE_IN_PIN);
    gpio_set_dir(PENDULUM_PULSE_IN_PIN, false);
    gpio_set_function(PENDULUM_PULSE_IN_PIN, GPIO_FUNC_PWM);
    const uint32_t slice_num_pendulum_in_pin = pwm_gpio_to_slice_num(PENDULUM_PULSE_IN_PIN);
    assert(slice_num_pendulum == slice_num_pendulum_in_pin);
    assert(pwm_gpio_to_channel(PENDULUM_PULSE_IN_PIN) == PWM_CHAN_B);

    // Configure for counting up

    uint16_t wrap = (uint16_t)round(10000.0 / frequency);

    pwm_config pwmConfigPendulum = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&pwmConfigPendulum, PWM_DIV_B_RISING);
    pwm_config_set_clkdiv_int_frac(&pwmConfigPendulum, 1, 0);
    pwm_config_set_wrap(&pwmConfigPendulum, wrap);

    pwm_init(slice_num_pendulum, &pwmConfigPendulum, false);

    uint16_t chanLevel = (uint16_t)round(highDuration * 10);

    pwm_set_chan_level(slice_num_pendulum, PWM_CHAN_A, chanLevel);

    pendulumSetEnabled(enabled);
}
