#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/util/datetime.h"

// For scb_hw so we can enable deep sleep
#include "hardware/structs/scb.h"

// For __wfi() function
#include "hardware/sync.h"

#include "hardware/rtc.h"

#include "StepperClock_Config.h"
#include "StepperClock_PWM_STEPPER.h"
#include "StepperClock_PWM_PENDULUM.h"
#include "StepperClock_Trinamic.h"
#include "StepperClock_RTC.h"

// #if !PICO_NO_FLASH
// #error "This code must be built to run from SRAM!"
// #endif

/// @brief Handles the usb power detection gpio pin going high.
/// @param gpio The gpio pin that caused the interrupt.
/// @param event_mask The type of interrupt that occured
void usbPowerDetectionHandler(uint gpio, uint32_t event_mask)
{
    if (gpio == 24 && event_mask == GPIO_IRQ_EDGE_RISE)
    {
        // Show that we are awake
        gpio_put(25, true);

        // Disable IRQ for rising edge
        gpio_set_irq_enabled(24, GPIO_IRQ_EDGE_RISE, false);

        // Disable sleep on exit of IRQ hanlder
        scb_hw->scr &= ~M0PLUS_SCR_SLEEPONEXIT_BITS;
    }
}

/// @brief Puts the current core to sleep until gpio pin 24 (usb power detection) goes high.
void goToSleep()
{
    // Enable IRQ for rising edge of usb power
    gpio_set_irq_enabled_with_callback(24, GPIO_IRQ_EDGE_RISE, true, usbPowerDetectionHandler);

    // Turn off all clocks when in sleep mode except for RTC
    // TODO: Check which other clocks need to keep running
    // clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS;
    // clocks_hw->sleep_en1 = 0x0;

    // Show that we went to sleep
    gpio_put(25, false);

    // Enable sleep and automatic sleep on exit from irq handler
    scb_hw->scr |= (M0PLUS_SCR_SLEEPDEEP_BITS | M0PLUS_SCR_SLEEPONEXIT_BITS);

    // Go to sleep
    __wfi();
}

/// @brief Reads a line from stdin. Aborts when the virtual serial console gets disconnected.
/// @param buffer The buffer to write the data into.
/// @param sizeOfBuffer The size of the buffer.
/// @return Number of chars read or zero when serial console got disconnected.
uint32_t readLine(char *buffer, uint32_t sizeOfBuffer)
{
    uint32_t index = 0;

    bool powerConnected = false;
    while (powerConnected = gpio_get(24) && index < sizeOfBuffer)
    {
        int c = getchar_timeout_us(1000000); // 1s

        if (c == PICO_ERROR_TIMEOUT)
            continue;

        if (c == '\n' || c == '\r')
        {
            puts("");
            break;
        }

        // Only printable ascii
        if (c < ' ' || c > '~')
            continue;

        putchar_raw(c);
        buffer[index++] = c;
    }

    // If power got disconnected before receiving a complete line return zero
    if (!powerConnected)
        return 0;

    return index;
}

/// @brief Converts chars 0 to 9 to an integer.
/// @param c The char to convert.
/// @param number The result of the converion.
/// @return true when the character was between 0 to 9, otherwise false.
bool convertCharToNumber(char c, uint32_t *number)
{
    if (c >= '0' && c <= '9')
    {
        *number = c - '0';
        return true;
    }

    return false;
}

/// @brief Parses a datetime with the given format dd.mm.yy hh:mm
/// @param buffer The buffer containing the string to parse.
/// @param sizeofBuffer The size of the buffer.
/// @param datetime The parsed date time.
/// @return true when the date time was parsed successfully, otherwise false
bool parseDateTime(char *buffer, uint32_t sizeofBuffer, datetime_t *datetime)
{
    if (sizeofBuffer != 14)
    {
        printf("Invalid length: %d\n", sizeofBuffer);
        return false;
    }

    uint32_t digit = 0;
    // Day
    if (!convertCharToNumber(buffer[0], &digit))
    {
        printf("Invalid char (day digit 1): %c\n", buffer[0]);
        return false;
    }
    datetime->day = digit;

    if (!convertCharToNumber(buffer[1], &digit))
    {
        printf("Invalid char (day digit 2): %c\n", buffer[1]);
        return false;
    }
    datetime->day = ((datetime->day * 10) + digit);

    if (buffer[2] != '.')
    {
        printf("Invalid char (day month seperator): %c\n", buffer[2]);
        return false;
    }

    // Month
    if (!convertCharToNumber(buffer[3], &digit))
    {
        printf("Invalid char (month digit 1): %c\n", buffer[3]);
        return false;
    }
    datetime->month = digit;

    if (!convertCharToNumber(buffer[4], &digit))
    {
        printf("Invalid char (month digit 2): %c\n", buffer[4]);
        return false;
    }
    datetime->month = ((datetime->month * 10) + digit);

    if (buffer[5] != '.')
    {
        printf("Invalid char (month year seperator): %c\n", buffer[5]);
        return false;
    }

    // Year
    if (!convertCharToNumber(buffer[6], &digit))
    {
        printf("Invalid char (year digit 1): %c\n", buffer[6]);
        return false;
    }
    datetime->year = digit;

    if (!convertCharToNumber(buffer[7], &digit))
    {
        printf("Invalid char (year digit 2): %c\n", buffer[7]);
        return false;
    }
    datetime->year = (2000 + ((datetime->year * 10) + digit));

    if (buffer[8] != ' ')
    {
        printf("Invalid char (year hour seperator): %c\n", buffer[8]);
        return false;
    }

    // Hour
    if (!convertCharToNumber(buffer[9], &digit))
    {
        printf("Invalid char (hour digit 1): %c\n", buffer[9]);
        return false;
    }
    datetime->hour = digit;

    if (!convertCharToNumber(buffer[10], &digit))
    {
        printf("Invalid char (hour digit 2): %c\n", buffer[10]);
        return false;
    }
    datetime->hour = ((datetime->hour * 10) + digit);

    if (datetime->hour > 23)
    {
        printf("Hour out of range (0-23): %02d", datetime->hour);
        return false;
    }

    if (buffer[11] != ':')
    {
        printf("Invalid char (hour minute seperator): %c\n", buffer[11]);
        return false;
    }

    // Minute
    if (!convertCharToNumber(buffer[12], &digit))
    {
        printf("Invalid char (minute digit 1): %c\n", buffer[12]);
        return false;
    }
    datetime->min = digit;

    if (!convertCharToNumber(buffer[13], &digit))
    {
        printf("Invalid char (minute digit 2): %c\n", buffer[12]);
        return false;
    }
    datetime->min = ((datetime->min * 10) + digit);

    if (datetime->min > 59)
    {
        printf("Minute out of range (0-59): %02d", datetime->min);
        return false;
    }

    datetime->sec = 0;
    datetime->dotw = 0;

    return true;
}

int main()
{
    // Wait 500ms before trying to init anything
    sleep_ms(500);

    configureTrinamicDrivers(TMC_UART, TMC_ENABLE_PIN, TMC_UART_TX_PIN, TMC_UART_RX_PIN);
    pendulumInit(PENDULUM_FREQUENCY, PENDULUM_PULSE_DURATION, true);
    datetime_t dateAndTime = {
        .year = 2023,
        .month = 05,
        .day = 10,
        .dotw = 0, // 0 is Sunday, so 5 is Friday
        .hour = 12,
        .min = 00,
        .sec = 00,
    };
    rtcInit(&dateAndTime);
    homeAndSeekClockHands();
    configurePwmForNormalOperation();

    // USB power detection gpio
    gpio_init(24);
    gpio_set_dir(24, false);

    // Onboard led
    gpio_init(25);
    gpio_set_dir(25, true);
    gpio_put(25, true);

    // If usb power isnt connected to to sleep
    if (!gpio_get(24))
        goToSleep();

    // Either we awoke from sleep or power was connected already
    while (true)
    {
        // Show that we are awake
        gpio_put(25, true);

        // Wait 500ms before attempting to do anything
        sleep_ms(500);

        if (CLOCK_RESTART_TRIGGER)
        {
            // Only trigger this once (when waking up)
            CLOCK_RESTART_TRIGGER = false;

            // Homing needs to be performed outisde of an IRQ handler since the homing process uses IRQs
            homeAndSeekClockHands();
            configurePwmForNormalOperation();

            // After homing is done jump to the end of the loop (which makes the pico go back to sleep)
            goto endOfLoop;
        }

        // While power is connected to the usb port try to init stdio through usb
        bool powerConnected = false;
        while (powerConnected = gpio_get(24))
        {
            if (stdio_usb_init())
                break;

            sleep_ms(1000);
        }
        if (!powerConnected)
            goto endOfLoop;

        // Wait until something has connected to the virtual serial port
        while (powerConnected = gpio_get(24))
        {
            if (stdio_usb_connected())
                break;

            sleep_ms(1000);
        }
        if (!powerConnected)
            goto endOfLoop;

        // Stay in the "set time" loop until power is disconnected
        while (gpio_get(24))
        {
            // Display start message
            puts("StepperClock V1.0 (Release)");
            rtc_get_datetime(&dateAndTime);
            printf("Current date and time: %02d.%02d.%d %02d:%02d\n",
                   dateAndTime.day,
                   dateAndTime.month,
                   dateAndTime.year,
                   dateAndTime.hour,
                   dateAndTime.min);
            puts("Press enter to continue.");

            // Wait until enter is pressed
            while (powerConnected = gpio_get(24))
            {
                int c = getchar_timeout_us(1000000); // 1s

                if (c == PICO_ERROR_TIMEOUT)
                    continue;

                if (c == '\n' || c == '\r')
                    break;
            }
            if (!powerConnected)
                goto endOfLoop;

            char buffer[128];

            // Display prompt for setting the date and time
            puts("Type date and time (dd.mm.yy hh:mm) and press enter.");
            while (powerConnected = gpio_get(24))
            {
                uint32_t numberOfCharsRead = readLine(buffer, count_of(buffer));

                // If nothing has been read either the power has been disconnected or enter was pressed immediately
                if (numberOfCharsRead == 0)
                    continue;

                bool success = parseDateTime(buffer, numberOfCharsRead, &dateAndTime);
                if (success)
                {
                    printf("Got date and time: %02d.%02d.%d %02d:%02d\n",
                           dateAndTime.day,
                           dateAndTime.month,
                           dateAndTime.year,
                           dateAndTime.hour,
                           dateAndTime.min);
                    break;
                }

                puts("Invalid input!");
            }
            if (!powerConnected)
                goto endOfLoop;

            rtcInit(&dateAndTime);
            homeAndSeekClockHands();
            configurePwmForNormalOperation();

            puts("Time set successfully!");
            sleep_ms(500);
        }

    endOfLoop:
        goToSleep();
    }

    return 0;
}
