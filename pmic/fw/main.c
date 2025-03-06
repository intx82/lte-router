#include "ch32v003fun.h"
#include "i2c_slave.h"
#include "ws2812.h"
#include <stdio.h>
#include <string.h>

#define BTN_PIN 3
#define ENA_PIN 4
#define TP4056_CHRG_PIN 5
#define TP4056_STDBY_PIN 6
#define LTE_LED_PIN 1
#define BAT_LOW_SH_TIME 10
#define BAT_LOW_ADC_THRESH 560
#define ADC_MEAS_INT 5000
#define BTN_LED_COUNTER 1000

#define I2C_DEV_ADDR 9

#define I2C_REG_TM        4
#define I2C_REG_LED_R     8
#define I2C_REG_LED_G     9
#define I2C_REG_LED_B    10
#define I2C_REG_LED_UPD  11
#define I2C_REG_ADC      12
#define I2C_REG_IN_STATE 14
#define I2C_REG_UID      16
#define I2C_REG_OFF      31

typedef struct in_state
{
    uint8_t charge : 1;
    uint8_t stdby : 1;
    uint8_t lte : 1;
    uint8_t pwr : 1;
    uint8_t bat_low : 1;
    uint8_t : 3;
} in_state_t;

static uint8_t i2c_registers[32] = {0x00};

void onWrite(uint8_t reg, uint8_t length)
{
    funDigitalWrite(PA2, i2c_registers[0] & 1);
    if ((i2c_registers[I2C_REG_OFF] & 0xff) == 0xff) {
        GPIOD->OUTDR &= ~(1 << ENA_PIN);
    }
}

uint32_t WS2812BLEDCallback(int ledno)
{
    return (i2c_registers[10]) | (i2c_registers[9] << 8) | (i2c_registers[8] << 16);
}

static void adc_init(void)
{
    RCC->CFGR0 &= ~(0x1F << 11);
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1;
    GPIOA->CFGLR &= ~(0xf << (4 * 2));
    RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;
    ADC1->RSQR1 = 0;
    ADC1->RSQR2 = 0;
    ADC1->RSQR3 = 0;
    ADC1->SAMPTR2 &= ~(ADC_SMP0 << (3 * 0));
    ADC1->SAMPTR2 |= 7 << (3 * 0);
    ADC1->CTLR2 |= ADC_ADON | ADC_EXTSEL;
    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL)
        ;

    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL)
        ;
}

uint16_t adc_get(void)
{
    ADC1->CTLR2 |= ADC_SWSTART;
    while (!(ADC1->STATR & ADC_EOC))
        ;
    return ADC1->RDATAR;
}

int main()
{
    SystemInit();
    funGpioInitAll();

    // Enable GPIOs
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

    GPIOD->CFGLR &= ~(0xf << (4 * BTN_PIN));
    GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4 * BTN_PIN);
    GPIOD->OUTDR |= (1 << BTN_PIN);

    GPIOD->CFGLR &= ~(0xf << (4 * TP4056_CHRG_PIN));
    GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4 * TP4056_CHRG_PIN);
    GPIOD->OUTDR |= (1 << TP4056_CHRG_PIN);

    GPIOD->CFGLR &= ~(0xf << (4 * TP4056_STDBY_PIN));
    GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_PUPD) << (4 * TP4056_STDBY_PIN);
    GPIOD->OUTDR |= (1 << TP4056_STDBY_PIN);

    GPIOA->CFGLR &= ~(0xf << (4 * LTE_LED_PIN));
    GPIOA->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * LTE_LED_PIN);

    GPIOD->CFGLR &= ~(0xf << (4 * ENA_PIN));
    GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * ENA_PIN);
    WS2812BDMAInit();
    i2c_registers[I2C_REG_LED_R] = 0xff;
    i2c_registers[I2C_REG_LED_G] = 0xff;
    i2c_registers[I2C_REG_LED_B] = 0xff;
    WS2812BDMAStart(1);

    Delay_Ms(1000);
    GPIOD->OUTDR |= (1 << ENA_PIN); // Turn on dev

    funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP);    // LED
    funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
    funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL

    SetupI2CSlave(I2C_DEV_ADDR, i2c_registers,
                  sizeof(i2c_registers), onWrite, NULL, false);

    adc_init();

    uint32_t *tm_reg = (uint32_t *)&i2c_registers[I2C_REG_TM];
    while ((GPIOD->INDR & (1 << BTN_PIN)) == 0) {
        if ((*tm_reg % ADC_MEAS_INT) >= 1000) {
            i2c_registers[I2C_REG_LED_R] = 0x40;
            i2c_registers[I2C_REG_LED_G] = 0x00;
            i2c_registers[I2C_REG_LED_B] = 0x40;
            WS2812BDMAStart(1);
        }
        Delay_Ms(50);
        (*tm_reg) += 50;
    }

    memset(i2c_registers, 0, sizeof(i2c_registers));
    memcpy(&i2c_registers[I2C_REG_UID], (uint8_t*) &ESIG->UID0, sizeof(uint32_t) * 3);
    i2c_registers[I2C_REG_LED_R] = 0x30;
    i2c_registers[I2C_REG_LED_G] = 0x20;
    i2c_registers[I2C_REG_LED_B] = 0x10;
    i2c_registers[I2C_REG_LED_UPD] = 1;
    printf("Started! \r\n");
    uint8_t low_voltage_counter = BAT_LOW_SH_TIME;

    while (1) {
        in_state_t *in_state = (in_state_t *)&i2c_registers[I2C_REG_IN_STATE];

        if (i2c_registers[I2C_REG_LED_UPD] > 0) {
            printf("Update color \r\n");
            WS2812BDMAStart(1);
            i2c_registers[I2C_REG_LED_UPD] = 0;
        } else if ((*tm_reg % ADC_MEAS_INT) == 0) {
            uint16_t *val = ((uint16_t *)&i2c_registers[I2C_REG_ADC]);
            *val = adc_get();
            printf("Measure voltage: %d \r\n", *val);

            if (*val < BAT_LOW_ADC_THRESH) {
                low_voltage_counter -= 1;
                in_state->bat_low = 1;
                printf("Device will shutdown in: %d sec \r\n",
                       (low_voltage_counter * ADC_MEAS_INT) / 1000);
                if (low_voltage_counter == 0) {
                    GPIOD->OUTDR &= ~(1 << ENA_PIN);
                }
            } else {
                low_voltage_counter = BAT_LOW_SH_TIME;
            }
        } else {
            Delay_Ms(1);
        }

        in_state->charge = (GPIOD->INDR & (1 << TP4056_CHRG_PIN)) > 0;
        in_state->stdby = (GPIOD->INDR & (1 << TP4056_STDBY_PIN)) > 0;
        in_state->lte = (GPIOA->INDR & (1 << LTE_LED_PIN)) > 0;
        in_state->pwr = (GPIOD->INDR & (1 << BTN_PIN)) > 0;

        (*tm_reg) += 1;
    }
}
