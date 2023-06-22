#include "StepperClock_Config.h"

const uint8_t CLOCK_SLEEP_MODE_HOUR = 23;
const uint8_t CLOCK_WAKE_UP_HOUR = 05;

// Offsets are in seconds to the 12 o'clock position
// Example:
// After homing the minute hands points at 45 minutes and 30 seconds
// This means there are 14 minutes and 30 seconds left for it to reach the 12 o'clock position.
// The value for the offset is calculated as follows:
// 14 * 60 + 30 = 870
// The same thing applies to the hour hand (just pretend its the minute hand for calculating the offset)
const uint32_t HOME_POSITION_OFFSET_MINUTE = (46 * 60) + 27;
const uint32_t HOME_POSITION_OFFSET_HOUR = (11 * 60) + 30;

const uint32_t HOUR_HOME_PIN = 4;
const uint32_t HOUR_HOME_POWER_PIN = 7;
const uint32_t HOUR_PULSE_PIN = 10;
const uint32_t HOUR_PULSE_IN_PIN = 11;
const uint8_t TMC_HOUR_SLAVE_ADDRESS = 0;

const uint32_t MINUTE_HOME_PIN = 3;
const uint32_t MINUTE_HOME_POWER_PIN = 8;
const uint32_t MINUTE_PULSE_PIN = 14;
const uint8_t TMC_MINUTE_SLAVE_ADDRESS = 1;

const uint32_t PENDULUM_PULSE_PIN = 16;
const uint32_t PENDULUM_PULSE_IN_PIN = 1;
const uint32_t PENDULUM_BASE_PULSE_PIN = 2;

// Pendulum length of 350mm
const double PENDULUM_FREQUENCY = 0.8425;     // Hz
const double PENDULUM_PULSE_DURATION = 160.0; // ms

// Other ULN pins = 17, 18, 19, 20, 21, 22

uart_inst_t *TMC_UART = uart0;
const uint32_t TMC_ENABLE_PIN = 15;
const uint32_t TMC_UART_TX_PIN = 12;
const uint32_t TMC_UART_RX_PIN = 13;
