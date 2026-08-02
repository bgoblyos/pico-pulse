// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <inttypes.h> // Debugging only

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "rheostat.h"

#define I2C_DEV i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_SPD 100000

#define MON_ADDR 0x2E
#define LIM_ADDR 0x2F

int mon_state = 0;
int lim_state = 0;

// TODO: make it so a failure here prevents the laser driver from turning on.
void init_rheostats(void) {
    setup_i2c();

    // Disable write protect
    uint8_t transmit[2] = {0x1C, 0x02}; // Sets write protect bit to 1 (write enabled)
    i2c_write_timeout_us(I2C_DEV, MON_ADDR, transmit, 2, false, 100000);
    i2c_write_timeout_us(I2C_DEV, LIM_ADDR, transmit, 2, false, 100000);

    // Initilaize limits to safe values
    set_monitor_current(25e-6); // 25 uA
    set_current_limit(55e-3);   // 55 mA
}

void setup_i2c() {
    i2c_init(I2C_DEV, I2C_SPD);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// Test functions from the SDK examples
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void bus_scan() {
    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(I2C_DEV, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}

int set_rheostat_position(uint8_t addr, int value) {
    // Do some bounds checking
    if (value >= (1 << 8) || value < 0)
        return PICO_ERROR_INVALID_DATA;

    uint16_t cmd = (1 << 10) | (value << 2);
    uint8_t transmit[] = {
        (uint8_t)(cmd >> 8),
        (uint8_t)(cmd & 0x00FF),
    };

    printf("Bytes sent: %" PRIu8 ", %" PRIu8 "\n", transmit[0], transmit[1]);

    return i2c_write_timeout_us(I2C_DEV, addr, transmit, 2, false, 100000);
}

float state_to_monitor(int state) {
    return 0.51 / (state * 2e4 / 255.0 + 1e3);
}

float state_to_limit(int state) {
    float resistance = 2e4 * state / 255.0;
    resistance = (resistance + 330)*1e4/(resistance + 330 + 1e4);
    return 700*0.52/resistance;
}

// Set the monitor rheostat in order to achieve the specified target current
float set_monitor_current(float current_A) {
    // Clamp input to reasonable range
    if (current_A > 0.00051)
        current_A = 0.00051;
    else if (current_A < 2.42857e-5)
        current_A = 2.42857e-5;

    float resistance = (0.51 / current_A) - 1000;
    int requested = round(255*(resistance/2e4));

    if (requested < 0)
        requested = 0;
    else if (requested > 255)
        requested = 255;

    printf("Current: %f, Resistance: %f Ohm\nRheostat position: %" PRIu8 "\n", current_A, resistance, requested);

    int retcode = set_rheostat_position(MON_ADDR, requested);

    if (retcode < 0)
        return retcode;
    else {
        mon_state = requested;
        return state_to_monitor(requested);
    }
}

float set_current_limit(float current_A) {
    // Clamp input to reasonable range
    if (current_A > 1.1394)
        current_A = 1.1394;
    else if (current_A < 0.054305)
        current_A = 0.054305;

    float resistance = (364 / (current_A - 0.0364)) - 330;
    int requested = round(255*(resistance/2e4));

    if (requested < 0)
        requested = 0;
    else if (requested > 255)
        requested = 255;

    int retcode = set_rheostat_position(LIM_ADDR, requested);
    if (retcode < 0)
        return retcode;
    else {
        lim_state = requested;
        return state_to_limit(requested);
    }
}

void set_current_limit_cmd(char* next_token){
    float target = atof(next_token);
    float result = set_current_limit(target);
    printf("%f\n", result);
}

void get_current_limit_cmd(void){
    float result = state_to_limit(lim_state);
    printf("%f\n", result);
}

void set_monitor_current_cmd(char* next_token){
    float target = atof(next_token);
    float result = set_monitor_current(target);
    printf("%f\n", result);
}

void get_monitor_current_cmd(void){
    float result = state_to_monitor(mon_state);
    printf("%f\n", result);
}
