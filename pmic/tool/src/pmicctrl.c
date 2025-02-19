/*
 * pmicctl.c - A user-space tool to control a DIY PMIC via I²C.
 *
 * This version includes a daemon mode that polls the PMIC’s power button
 * and publishes ubus events, and also registers a ubus object to listen for
 * commands (e.g. to set the LED color).
 *
 * Supported commands:
 *   read [--json]        - Read PMIC registers and display values.
 *   set-led <R> <G> <B>   - Immediately set the LED color.
 *   shutdown             - Send shutdown command via I²C.
 *   daemon               - Run as a daemon: poll power-button and handle ubus requests.
 *   version              - Print version information.
 *
 * Compile along with your i2c.c code.
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i2c.h"
#include "version.hpp"
#include "daemon.h"


/* I2C bus and PMIC device definitions */
#define I2C_BUS "/dev/i2c-0"
#define PMIC_ADDR 0x09
#define PMIC_REG_COUNT 32

/* Function prototypes */
void print_usage(const char *progname);
int read_registers_text(struct I2cDevice *dev);
int read_registers_json(struct I2cDevice *dev);
int shutdown_device(struct I2cDevice *dev);


#define dbg() printf("%s:%d\r\n", __FILE__, __LINE__)


/* --- Other functions (read, set-led, shutdown, etc.) --- */

void print_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <command> [arguments]\n", progname);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  read [--json]        - Read PMIC registers and display values\n");
    fprintf(stderr, "  set-led <R> <G> <B>   - Set LED color (each value in hex or decimal)\n");
    fprintf(stderr, "  shutdown             - Send shutdown command via I2C\n");
    fprintf(stderr, "  daemon               - Run daemon (polls power button and listens for ubus commands)\n");
    fprintf(stderr, "  version              - Print version information\n");
}

int read_registers_text(struct I2cDevice *dev)
{
    uint8_t regs[PMIC_REG_COUNT];
    int rc = i2c_readn_reg(dev, 0, regs, PMIC_REG_COUNT);
    if (rc <= 0) {
        fprintf(stderr, "Failed to read PMIC registers\n");
        return -1;
    }

    printf("PMIC Register Dump:\n");
    for (int i = 0; i < PMIC_REG_COUNT; i++) {
        printf(" Reg %2d: 0x%02x\n", i, regs[i]);
    }

    uint32_t tm = (regs[7] << 24) | (regs[6] << 16) | (regs[5] << 8) | regs[4];
    uint16_t adc_val = (regs[13] << 8) | regs[12];
    uint8_t in_state = regs[14];
    float vbat = DIV_RATIO * (VREF * ((float)adc_val / ADC_MAX));

    printf("\nDecoded Fields:\n");
    printf("  Time (ms): %u\n", tm);
    printf("  LED Color: R=0x%02x, G=0x%02x, B=0x%02x (trigger=0x%02x)\n",
           regs[8], regs[9], regs[10], regs[11]);
    printf("  ADC Value: %u\n", adc_val);
    printf("  Battery Voltage: %.3f V\n", vbat);
    printf("  In-State : 0x%02x\n", in_state);

    return 0;
}

int read_registers_json(struct I2cDevice *dev)
{
    uint8_t regs[PMIC_REG_COUNT];
    int rc = i2c_readn_reg(dev, 0, regs, PMIC_REG_COUNT);
    if (rc <= 0) {
        fprintf(stderr, "Failed to read PMIC registers\n");
        return -1;
    }

    uint32_t tm = (regs[7] << 24) | (regs[6] << 16) | (regs[5] << 8) | regs[4];
    uint8_t r = regs[8], g = regs[9], b = regs[10], trigger = regs[11];
    uint16_t adc_val = (regs[13] << 8) | regs[12];
    uint8_t in_state = regs[14];
    float vbat = DIV_RATIO * (VREF * ((float)adc_val / ADC_MAX));

    printf("{\n");
    printf("  \"tm\": %u,\n", tm);
    printf("  \"led_color\": {\n");
    printf("    \"r\": %u,\n", r);
    printf("    \"g\": %u,\n", g);
    printf("    \"b\": %u,\n", b);
    printf("    \"trigger\": %u\n", trigger);
    printf("  },\n");
    printf("  \"adc_val\": %u,\n", adc_val);
    printf("  \"vbat\": %.3f,\n", vbat);
    printf("  \"in_state\": %u\n", in_state);
    printf("}\n");

    return 0;
}

int set_led_color(struct I2cDevice *dev, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t data[4] = {r, g, b, 0x01};
    int rc = i2c_writen_reg(dev, 8, data, 4);
    if (rc < 0) {
        fprintf(stderr, "Failed to write LED color\n");
        return -1;
    }
    printf("Set LED color to: R=0x%02x, G=0x%02x, B=0x%02x\n", r, g, b);
    return 0;
}

int shutdown_device(struct I2cDevice *dev)
{
    int rc = i2c_write_reg(dev, 31, 0xff);
    if (rc < 0) {
        fprintf(stderr, "Failed to send shutdown command\n");
        return -1;
    }
    printf("Shutdown command sent.\n");
    return 0;
}

/* Main function: command dispatch */
int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("%s\n", VERSION);
        return EXIT_SUCCESS;
    }

    /* Initialize the I2C device */
    struct I2cDevice dev;
    dev.filename = I2C_BUS;
    dev.addr = PMIC_ADDR;

    if (i2c_start(&dev) < 0) {
        perror("i2c_start failed");
        return EXIT_FAILURE;
    }

    /* Dispatch commands */
    if (strcmp(argv[1], "read") == 0) {
        if (argc > 2 && strcmp(argv[2], "--json") == 0)
            ret = read_registers_json(&dev);
        else
            ret = read_registers_text(&dev);
    } else if (strcmp(argv[1], "set-led") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Error: set-led requires 3 arguments: R G B\n");
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
        } else {
            uint8_t r = (uint8_t)strtol(argv[2], NULL, 0);
            uint8_t g = (uint8_t)strtol(argv[3], NULL, 0);
            uint8_t b = (uint8_t)strtol(argv[4], NULL, 0);
            ret = set_led_color(&dev, r, g, b);
        }
    } else if (strcmp(argv[1], "shutdown") == 0) {
        ret = shutdown_device(&dev);
    } else if (strcmp(argv[1], "daemon") == 0) {
        ret = run_daemon(&dev);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
    }

    i2c_stop(&dev);
    return ret;
}
