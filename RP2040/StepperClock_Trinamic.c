#include "pico/time.h"

#include "tmc/ic/TMC2209/TMC2209.h"
#include "tmc/helpers/Config.h"

#include "hardware/gpio.h"

#include "StepperClock_Config.h"
#include "StepperClock_Trinamic.h"

void configureTrinamicDriver(TMC2209TypeDef *tmc)
{
    // StealthChop PWM mode enabled
    TMC2209_FIELD_UPDATE(tmc, TMC2209_GCONF, TMC2209_EN_SPREADCYCLE_MASK, TMC2209_EN_SPREADCYCLE_SHIFT, 0);

    // PDN_UART input function disabled. Set this bit, when using the UART interface!
    TMC2209_FIELD_UPDATE(tmc, TMC2209_GCONF, TMC2209_PDN_UART_MASK, TMC2209_PDN_UART_SHIFT, 1);

    // Inverse motor direction
    // TMC2209_FIELD_UPDATE(tmc, TMC2209_GCONF, TMC2209_SHAFT_MASK, TMC2209_SHAFT_SHIFT, 0);

    // Microstep resolution selected by MRES register
    TMC2209_FIELD_UPDATE(tmc, TMC2209_GCONF, TMC2209_MSTEP_REG_SELECT_MASK, TMC2209_MSTEP_REG_SELECT_SHIFT, 1);

    // Standstill current (0=1/32 … 31=32/32)
    TMC2209_FIELD_UPDATE(tmc, TMC2209_IHOLD_IRUN, TMC2209_IHOLD_MASK, TMC2209_IHOLD_SHIFT, 6);

    // Motor run current (0=1/32 … 31=32/32)
    TMC2209_FIELD_UPDATE(tmc, TMC2209_IHOLD_IRUN, TMC2209_IRUN_MASK, TMC2209_IRUN_SHIFT, 12);

    // Delay per current reduction step in multiple of 2^18 clocks
    TMC2209_FIELD_UPDATE(tmc, TMC2209_IHOLD_IRUN, TMC2209_IHOLDDELAY_MASK, TMC2209_IHOLDDELAY_SHIFT, 1);

    // Sets the delay time from stand still (stst) detection to motor current power down.
    TMC2209_FIELD_UPDATE(tmc, TMC2209_TPOWERDOWN, TMC2209_TPOWERDOWN_MASK, TMC2209_TPOWERDOWN_SHIFT, 5);

    // Sets the upper velocity for StealthChop voltage PWM mode 0: Disabled
    TMC2209_FIELD_UPDATE(tmc, TMC2209_TPWMTHRS, TMC2209_TPWMTHRS_MASK, TMC2209_TPWMTHRS_SHIFT, 0);

    // Native 256 microstep setting
    TMC2209_FIELD_UPDATE(tmc, TMC2209_CHOPCONF, TMC2209_MRES_MASK, TMC2209_MRES_SHIFT, 0);

    int32_t value = tmc2209_readInt(tmc, TMC2209_IOIN);
    int32_t enn = FIELD_GET(value, TMC2209_ENN_MASK, TMC2209_ENN_SHIFT);
    int32_t ms1 = FIELD_GET(value, TMC2209_MS1_MASK, TMC2209_MS1_SHIFT);
    int32_t ms2 = FIELD_GET(value, TMC2209_MS2_MASK, TMC2209_MS2_SHIFT);
    int32_t diag = FIELD_GET(value, TMC2209_DIAG_MASK, TMC2209_DIAG_SHIFT);
    int32_t pdn_uart = FIELD_GET(value, TMC2209_PDN_UART_MASK, TMC2209_PDN_UART_SHIFT);
    int32_t step = FIELD_GET(value, TMC2209_STEP_MASK, TMC2209_STEP_SHIFT);
    int32_t spread = FIELD_GET(value, TMC2209_SEL_A_MASK, TMC2209_SEL_A_SHIFT);
    int32_t dir = FIELD_GET(value, TMC2209_DIR_MASK, TMC2209_DIR_SHIFT);
    int32_t version = FIELD_GET(value, TMC2209_VERSION_MASK, TMC2209_VERSION_SHIFT);

    // printf("ioin: %i, enn: %i, ms1: %i, ms2: %i, diag: %i, pdn_uart: %i, step: %i, spread: %i, dir: %i, version: %i\n", value, enn, ms1, ms2, diag, pdn_uart, step, spread, dir, version);

    value = tmc2209_readInt(tmc, TMC2209_DRVSTATUS);

    int32_t stst = FIELD_GET(value, TMC2209_STST_MASK, TMC2209_STST_SHIFT);
    int32_t stealth = FIELD_GET(value, TMC2209_STEALTH_MASK, TMC2209_STEALTH_SHIFT);
    int32_t cs_actual = FIELD_GET(value, TMC2209_CS_ACTUAL_MASK, TMC2209_CS_ACTUAL_SHIFT);
    int32_t t157 = FIELD_GET(value, TMC2209_T157_MASK, TMC2209_T157_SHIFT);
    int32_t t150 = FIELD_GET(value, TMC2209_T150_MASK, TMC2209_T150_SHIFT);
    int32_t t143 = FIELD_GET(value, TMC2209_T143_MASK, TMC2209_T143_SHIFT);
    int32_t t120 = FIELD_GET(value, TMC2209_T120_MASK, TMC2209_T120_SHIFT);
    int32_t olb = FIELD_GET(value, TMC2209_OLB_MASK, TMC2209_OLB_SHIFT);
    int32_t ola = FIELD_GET(value, TMC2209_OLA_MASK, TMC2209_OLA_SHIFT);
    int32_t s2vsb = FIELD_GET(value, TMC2209_S2VSB_MASK, TMC2209_S2VSB_SHIFT);
    int32_t s2vsa = FIELD_GET(value, TMC2209_S2VSA_MASK, TMC2209_S2VSA_SHIFT);
    int32_t s2gb = FIELD_GET(value, TMC2209_S2GB_MASK, TMC2209_S2GB_SHIFT);
    int32_t s2ga = FIELD_GET(value, TMC2209_S2GA_MASK, TMC2209_S2GA_SHIFT);
    int32_t ot = FIELD_GET(value, TMC2209_OT_MASK, TMC2209_OT_SHIFT);
    int32_t otpw = FIELD_GET(value, TMC2209_OTPW_MASK, TMC2209_OTPW_SHIFT);

    // printf("drvstatus: %i, stst: %i, stealth: %i, cs_actual: %i, t157: %i, t150: %i, t143: %i, t120: %i, olb: %i, ola: %i, s2vsb: %i, s2vsa: %i, s2gb: %i, s2ga: %i, ot: %i, otpw: %i\n",
    //        value, stst, stealth, cs_actual, t157, t150, t143, t120, olb, ola, s2vsb, s2vsa, s2gb, s2ga, ot, otpw);
}

void configureTrinamicDrivers(uart_inst_t *uart, uint32_t enablePin, uint32_t txPin, uint32_t rxPin)
{
    gpio_init(enablePin);
    gpio_set_dir(enablePin, true);
    gpio_put(enablePin, true);

    gpio_init(txPin);
    gpio_set_dir(txPin, true);

    gpio_init(rxPin);
    gpio_set_dir(rxPin, false);

    uart_init(uart, 38400);

    gpio_set_function(txPin, GPIO_FUNC_UART);
    gpio_set_function(rxPin, GPIO_FUNC_UART);

    uart_set_hw_flow(uart, false, false);
    uart_set_format(uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart, true);
    uart_set_translate_crlf(uart, false);

    tmc_fillCRC8Table(0x07, true, 0);
    tmc_fillCRC8Table(0x07, true, 1);

    ////////////////////////////////////////////////

    ConfigurationTypeDef tmcHourHandConfig;
    TMC2209TypeDef tmcHourHand;
    tmc2209_init(&tmcHourHand, 0, TMC_HOUR_SLAVE_ADDRESS, &tmcHourHandConfig, tmc2209_defaultRegisterResetState);
    configureTrinamicDriver(&tmcHourHand);

    ////////////////////////////////////////////////

    // Wait between configuring drivers
    sleep_ms(250);

    ////////////////////////////////////////////////

    ConfigurationTypeDef tmcHourMinuteHandConfig;
    TMC2209TypeDef tmcMinuteHand;
    tmc2209_init(&tmcMinuteHand, 0, TMC_MINUTE_SLAVE_ADDRESS, &tmcHourMinuteHandConfig, tmc2209_defaultRegisterResetState);
    configureTrinamicDriver(&tmcMinuteHand);

    ////////////////////////////////////////////////

    gpio_put(enablePin, false);
}