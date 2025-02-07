/*
 * pmicctl.c - A simple user-space tool to control a DIY PMIC via I²C.
 *
 * This tool uses the I²C helper code provided in i2c.c.
 *
 * It communicates with a PMIC device on bus 0 (/dev/i2c-0) at address 9.
 *
 * Supported commands:
 *   read [--json]        - Read and display PMIC registers.
 *                          Without --json: prints a hex dump and decoded fields.
 *                          With --json: outputs the parsed fields as a JSON object.
 *   set-led <R> <G> <B>   - Set LED color (each value in hex or decimal).
 *                          The update is triggered by writing a nonzero trigger byte.
 *   shutdown             - Write 0xff to the shutdown register to power off.
 *   version              - Print version information.
 *
 * Example usage:
 *   ./pmicctl read --json
 *   ./pmicctl set-led 0xff 0x00 0x80
 *   ./pmicctl shutdown
 *   ./pmicctl version
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "version.hpp"

#include "i2c.h"


/* I2C bus and PMIC device address definitions */
#define I2C_BUS         "/dev/i2c-0"
#define PMIC_ADDR       0x09
#define PMIC_REG_COUNT  32

/* ADC and voltage divider parameters */
#define VREF        3.3f
#define ADC_MAX     1024.0f
#define DIV_RATIO   2.0f

/* Function prototypes */
void print_usage(const char *progname);
int read_registers_text(struct I2cDevice* dev);
int read_registers_json(struct I2cDevice* dev);
int set_led_color(struct I2cDevice* dev, uint8_t r, uint8_t g, uint8_t b);
int shutdown_device(struct I2cDevice* dev);

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    /* If the user asks for the version, print it and exit */
    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("%s\n", VERSION);
        return EXIT_SUCCESS;
    }
    
    /* Initialize the I2C device structure */
    struct I2cDevice dev;
    dev.filename = I2C_BUS;
    dev.addr = PMIC_ADDR;
    
    /* Start the I2C device (open the bus and set the slave address) */
    if (i2c_start(&dev) < 0) {
        perror("i2c_start failed");
        return EXIT_FAILURE;
    }
    
    /* Command dispatch */
    if (strcmp(argv[1], "read") == 0) {
        /* Use JSON output if the flag is provided */
        if (argc > 2 && strcmp(argv[2], "--json") == 0)
            ret = read_registers_json(&dev);
        else
            ret = read_registers_text(&dev);
    }
    else if (strcmp(argv[1], "set-led") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Error: set-led requires 3 arguments: R G B\n");
            print_usage(argv[0]);
            ret = EXIT_FAILURE;
        }
        else {
            /* Parse the R, G, B values (hex or decimal) */
            uint8_t r = (uint8_t)strtol(argv[2], NULL, 0);
            uint8_t g = (uint8_t)strtol(argv[3], NULL, 0);
            uint8_t b = (uint8_t)strtol(argv[4], NULL, 0);
            ret = set_led_color(&dev, r, g, b);
        }
    }
    else if (strcmp(argv[1], "shutdown") == 0) {
        ret = shutdown_device(&dev);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        ret = EXIT_FAILURE;
    }
    
    /* Stop the I2C device (close the bus file descriptor) */
    i2c_stop(&dev);
    return ret;
}

/* Print usage information */
void print_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <command> [arguments]\n", progname);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  read [--json]        - Read PMIC registers and display values\n");
    fprintf(stderr, "                         Use --json for JSON output\n");
    fprintf(stderr, "  set-led <R> <G> <B>   - Set LED color (each value in hex or decimal)\n");
    fprintf(stderr, "  shutdown             - Send shutdown command\n");
    fprintf(stderr, "  version              - Print version information\n");
}

/*
 * read_registers_text:
 *
 * Reads all 32 PMIC registers starting at register 0.
 * It prints a hex dump of all registers and then decodes a few fields:
 *
 *   - tm: registers 4..7 (uint32_t, little-endian)
 *   - led_color: registers 8..11 (R, G, B, Trigger)
 *   - adc_val: registers 12..13 (uint16_t, little-endian)
 *   - in_state: register 14 (uint8_t)
 *
 * Also calculates the battery voltage:
 *    vbat = DIV_RATIO * (VREF * (adc_val / ADC_MAX))
 */
int read_registers_text(struct I2cDevice* dev)
{
    uint8_t regs[PMIC_REG_COUNT];
    int rc = i2c_readn_reg(dev, 0, regs, PMIC_REG_COUNT);
    if (rc <= 0) {
        fprintf(stderr, "Failed to read PMIC registers\n");
        return EXIT_FAILURE;
    }
    
    printf("PMIC Register Dump:\n");
    for (int i = 0; i < PMIC_REG_COUNT; i++) {
        printf(" Reg %2d: 0x%02x\n", i, regs[i]);
    }
    
    /* Decode multi-byte fields (little-endian) */
    uint32_t tm = (regs[7] << 24) | (regs[6] << 16) | (regs[5] << 8) | regs[4];
    uint16_t adc_val = (regs[13] << 8) | regs[12];
    uint8_t in_state = regs[14];
    
    /* Calculate battery voltage */
    float vbat = DIV_RATIO * (VREF * ((float)adc_val / ADC_MAX));
    
    printf("\nDecoded Fields:\n");
    printf("  Time (ms): %u\n", tm);
    printf("  LED Color: R=0x%02x, G=0x%02x, B=0x%02x (trigger=0x%02x)\n",
           regs[8], regs[9], regs[10], regs[11]);
    printf("  ADC Value: %u\n", adc_val);
    printf("  Battery Voltage: %.3f V\n", vbat);
    printf("  In-State : 0x%02x\n", in_state);
    
    return EXIT_SUCCESS;
}

/*
 * read_registers_json:
 *
 * Reads all 32 PMIC registers starting at register 0 and outputs the parsed
 * fields as a JSON object:
 *
 * {
 *   "tm": <value>,
 *   "led_color": { "r": <value>, "g": <value>, "b": <value>, "trigger": <value> },
 *   "adc_val": <value>,
 *   "vbat": <calculated battery voltage>,
 *   "in_state": <value>
 * }
 */
int read_registers_json(struct I2cDevice* dev)
{
    uint8_t regs[PMIC_REG_COUNT];
    int rc = i2c_readn_reg(dev, 0, regs, PMIC_REG_COUNT);
    if (rc <= 0) {
        fprintf(stderr, "Failed to read PMIC registers\n");
        return EXIT_FAILURE;
    }
    
    uint32_t tm = (regs[7] << 24) | (regs[6] << 16) | (regs[5] << 8) | regs[4];
    uint8_t r = regs[8];
    uint8_t g = regs[9];
    uint8_t b = regs[10];
    uint8_t trigger = regs[11];
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
    
    return EXIT_SUCCESS;
}

/*
 * set_led_color:
 *
 * Writes 4 bytes starting at register 8:
 *   - Register 8: red component
 *   - Register 9: green component
 *   - Register 10: blue component
 *   - Register 11: trigger (nonzero to update the LED)
 */
int set_led_color(struct I2cDevice* dev, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t data[4] = { r, g, b, 0x01 };
    int rc = i2c_writen_reg(dev, 8, data, 4);
    if (rc < 0) {
        fprintf(stderr, "Failed to write LED color\n");
        return EXIT_FAILURE;
    }
    printf("Set LED color to: R=0x%02x, G=0x%02x, B=0x%02x\n", r, g, b);
    return EXIT_SUCCESS;
}

/*
 * shutdown_device:
 *
 * Writes 0xff (one byte) to register 31 to initiate a shutdown.
 */
int shutdown_device(struct I2cDevice* dev)
{
    int rc = i2c_write_reg(dev, 31, 0xff);
    if (rc < 0) {
        fprintf(stderr, "Failed to send shutdown command\n");
        return EXIT_FAILURE;
    }
    printf("Shutdown command sent.\n");
    return EXIT_SUCCESS;
}
