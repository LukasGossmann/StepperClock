#ifndef STEPPER_CLOCK_CONFIG_H
#define STEPPER_CLOCK_CONFIG_H

#include <stdint.h>
#include "hardware/uart.h"

extern const uint8_t CLOCK_SLEEP_MODE_HOUR;
extern const uint8_t CLOCK_WAKE_UP_HOUR;

extern const uint32_t HOME_POSITION_OFFSET_HOUR;
extern const uint32_t HOME_POSITION_OFFSET_MINUTE;

extern const uint32_t HOUR_HOME_PIN;
extern const uint32_t HOUR_HOME_POWER_PIN;
extern const uint32_t HOUR_PULSE_PIN;
extern const uint32_t HOUR_PULSE_IN_PIN;
extern const uint8_t TMC_HOUR_SLAVE_ADDRESS;

extern const uint32_t MINUTE_HOME_PIN;
extern const uint32_t MINUTE_HOME_POWER_PIN;
extern const uint32_t MINUTE_PULSE_PIN;
extern const uint8_t TMC_MINUTE_SLAVE_ADDRESS;

extern const uint32_t PENDULUM_PULSE_PIN;
extern const uint32_t PENDULUM_PULSE_IN_PIN;
extern const uint32_t PENDULUM_BASE_PULSE_PIN;

extern const double PENDULUM_FREQUENCY;
extern const double PENDULUM_PULSE_DURATION;

extern uart_inst_t *TMC_UART;
extern const uint32_t TMC_ENABLE_PIN;
extern const uint32_t TMC_UART_TX_PIN;
extern const uint32_t TMC_UART_RX_PIN;

#endif