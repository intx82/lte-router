#include "ch32v003fun.h"
#include <stdio.h>
#include "i2c_slave.h"

#define BTN_PIN 4
#define ENA_PIN 3

static uint8_t i2c_registers[32] = {0x00};

void onWrite(uint8_t reg, uint8_t length) {
    funDigitalWrite(PA2, i2c_registers[0] & 1);
    if ((i2c_registers[31] & 0xff) == 0xff) {
        GPIOD->OUTDR &= ~(1 << ENA_PIN);
    }
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

    GPIOD->CFGLR &= ~(0xf << (4 * ENA_PIN));
    GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * ENA_PIN);
    Delay_Ms(1000);
    GPIOD->OUTDR |= (1 << ENA_PIN); // Turn on dev

    funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP); // LED
    funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
    funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL
    SetupI2CSlave(0x9, i2c_registers, 32, onWrite, NULL, false);

    uint32_t* tm_reg = (uint32_t*)&i2c_registers[4];

    while ((GPIOD->INDR & (1 << BTN_PIN)) == 0);

    while (1) {
        if ((GPIOD->INDR & (1 << BTN_PIN)) == 0) {
            Delay_Ms(1000);
            if ((GPIOD->INDR & (1 << BTN_PIN)) == 0) {
                GPIOD->OUTDR &= ~(1 << ENA_PIN);
            }
        }
        Delay_Ms(1);
        tm_reg += 1;
    }
}
