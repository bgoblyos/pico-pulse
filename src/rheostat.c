// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <inttypes.h> // Debugging only

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "rheostat.h"

#include "board_config.h"

int mon_state = 0;
int lim_state = 0;

float mon_max_current = 0;
float mon_min_current = 0;
float lim_max_current = 0;
float lim_min_current = 0;

#if USE_LASER_DRIVER

// TODO: make it so a failure here prevents the laser driver from turning on.
// TODO: also make sure this is ran again on the next write of the initial setup
// failed
void init_rheostats(void) {
  setup_i2c();

  // Disable write protect
  uint8_t transmit[2] = {0x1C,
                         0x02}; // Sets write protect bit to 1 (write enabled)
  i2c_write_timeout_us(I2C_DEV, MON_ADDR, transmit, 2, false, 100000);
  i2c_write_timeout_us(I2C_DEV, LIM_ADDR, transmit, 2, false, 100000);

  // Calculate constants
  mon_max_current = state_to_monitor(0);
  mon_min_current = state_to_monitor(255);

  lim_max_current = state_to_limit(0);
  lim_min_current = state_to_limit(255);

  // Initilaize limits to safe values
  set_monitor_current(mon_min_current);
  set_current_limit(lim_min_current);
}

void setup_i2c() {
  i2c_init(I2C_DEV, I2C_SPD);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
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

  return i2c_write_timeout_us(I2C_DEV, addr, transmit, 2, false, 100000);
}

float state_to_monitor(int state) {
  return 0.51 / (state * 2e4 / 255.0 + LIM_SERIES_RES);
}

float state_to_limit(int state) {
  float resistance = 2e4 * state / 255.0;
  resistance = (resistance + 330) * 1e4 / (resistance + 330 + 1e4);
  return 700 * 0.52 / resistance;
}

// Set the monitor rheostat in order to achieve the specified target current
float set_monitor_current(float current_A) {
  // Clamp input to reasonable range
  if (current_A > mon_max_current)
    current_A = mon_max_current;
  else if (current_A < mon_min_current)
    current_A = mon_min_current;

  float resistance = (0.51 / current_A) - LIM_SERIES_RES;
  int requested = round(255 * (resistance / 2e4));

  if (requested < 0)
    requested = 0;
  else if (requested > 255)
    requested = 255;

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
  if (current_A > lim_max_current)
    current_A = lim_max_current;
  else if (current_A < lim_min_current)
    current_A = lim_min_current;

  float resistance = (364 / (current_A - 0.0364)) - 330;
  int requested = round(255 * (resistance / 2e4));

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

void set_current_limit_cmd(char *next_token) {
  float target = atof(next_token);
  float result = set_current_limit(target);
  printf("%f\n", result);
}

void get_current_limit_cmd(void) {
  float result = state_to_limit(lim_state);
  printf("%f\n", result);
}

void set_monitor_current_cmd(char *next_token) {
  float target = atof(next_token);
  float result = set_monitor_current(target);
  printf("%f\n", result);
}

void get_monitor_current_cmd(void) {
  float result = state_to_monitor(mon_state);
  printf("%f\n", result);
}

#else

void init_rheostats(void) {}

void setup_i2c() {}

int set_rheostat_position(uint8_t addr, int value) { return 0; }

float state_to_monitor(int state) { return 0.0; }

float state_to_limit(int state) { return 0.0; }

float set_monitor_current(float current_A) { return 0.0; }

float set_current_limit(float current_A) { return 0.0; }

void set_current_limit_cmd(char *next_token) {
  printf("ERR: This firmware was built without laser driver support.\n");
}

void get_current_limit_cmd(void) {
  printf("ERR: This firmware was built without laser driver support.\n");
}

void set_monitor_current_cmd(char *next_token) {
  printf("ERR: This firmware was built without laser driver support.\n");
}

void get_monitor_current_cmd(void) {
  printf("ERR: This firmware was built without laser driver support.\n");
}

#endif
