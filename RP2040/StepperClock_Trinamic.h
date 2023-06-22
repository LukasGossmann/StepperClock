#ifndef STEPPER_CLOCK_TRINAMIC_H
#define STEPPER_CLOCK_TRINAMIC_H

#include "hardware/uart.h"

void configureTrinamicDrivers(uart_inst_t *uart, uint32_t enablePin, uint32_t txPin, uint32_t rxPin);

#endif