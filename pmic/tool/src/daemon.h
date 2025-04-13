#ifndef __DAEMON_H
#define __DAEMON_H

#include "i2c.h"

#define VREF 3.3f
#define ADC_MAX 1024.0f
#define DIV_RATIO 2.0f

#define STATUS_POLL_INTERVAL 100 // ms
#define VBAT_POLL_INTERVAL 5000 // ms


typedef union
{
    uint8_t raw;
    struct
    {
        uint8_t charge : 1;
        uint8_t stdby : 1;
        uint8_t lte : 1;
        uint8_t pwr : 1;
        uint8_t bat_low : 1;
        uint8_t : 3;
    };
} pmic_state_t;

#define dbg() printf("%s:%d\r\n", __FILE__, __LINE__)

int run_daemon(struct I2cDevice *dev);

#endif