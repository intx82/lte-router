/*
 * Single-File-Header for using the I2C peripheral in slave mode
 *
 * MIT License
 *
 * Copyright (c) 2024 Renze Nicolai
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __I2C_SLAVE_H
#define __I2C_SLAVE_H

#include "ch32v003fun.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef void (*i2c_write_callback_t)(uint8_t reg, uint8_t length);
typedef void (*i2c_read_callback_t)(uint8_t reg);


void I2C1_ER_IRQHandler(void) __attribute__((interrupt));
void I2C1_EV_IRQHandler(void) __attribute__((interrupt));

void SetupI2CSlave(uint8_t address, volatile uint8_t* registers, uint8_t size, i2c_write_callback_t write_callback, i2c_read_callback_t read_callback, bool read_only);
void SetupSecondaryI2CSlave(uint8_t address, volatile uint8_t* registers, uint8_t size, i2c_write_callback_t write_callback, i2c_read_callback_t read_callback, bool read_only);

#endif
