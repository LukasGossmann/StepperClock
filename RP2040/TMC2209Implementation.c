#include <stdio.h>

#include "pico/time.h"

#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "tmc/helpers/CRC.h"

void tmc2209_readWriteArray(uint8_t channel, uint8_t *data, size_t writeLength, size_t readLength)
{
    uart_inst_t *uart = NULL;

    switch (channel)
    {
    case 0:
        uart = uart0;
        break;
    case 1:
        uart = uart1;
        break;
    }

    assert(uart != NULL);

    /*
    printf("Channel: %i\n", channel);

    printf("Write: ");
    for (size_t i = 0; i < writeLength; i++)
        printf("%i ", data[i]);
    puts("");
    */

    uart_hw_t *uart_hw = uart_get_hw(uart);

    // Disable uart rx module before transmission otherwise the stuff we sent
    // will clog up the rx fifo since tx and rx are linked together with a 1K resistor
    hw_clear_bits(&uart_hw->cr, UART_UARTCR_RXE_BITS);

    uart_write_blocking(uart, data, writeLength);

    // Wait until uart is not busy anymore which means all data has been sent
    while ((uart_hw->fr & UART_UARTFR_BUSY_BITS) == UART_UARTFR_BUSY_BITS)
        ;

    // Reenable rx module to receive the response from the trinamic driver
    hw_set_bits(&uart_hw->cr, UART_UARTCR_RXE_BITS);

    if (readLength <= 0)
        return;

    uart_read_blocking(uart, data, readLength);

    sleep_ms(1);

    /*
    printf("Read: ");
    for (size_t i = 0; i < readLength; i++)
        printf("%i ", data[i]);
    puts("");
    */
}

uint8_t tmc2209_CRC8(uint8_t *data, size_t length)
{
    return tmc_CRC8(data, length, 0);
}