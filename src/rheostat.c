// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

// Supports two AD5274BRM rheostats on an I2C bus

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "rheostat.h"

#include "board_config.h"

#define RHEOSTAT_TIMEOUT 100000

int mon_state = 255;
int lim_state = 255;

float mon_max_current = 0;
float mon_min_current = 0;
float lim_max_current = 0;
float lim_min_current = 0;

#if USE_LASER_DRIVER

void init_rheostats(void) {
  setup_i2c();
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

int wrap_i2c(uint8_t addr, uint8_t *buffer, size_t len, bool write) {
  int retcode;

  if (write)
    retcode = i2c_write_timeout_us(I2C_DEV, addr, buffer, len, false,
                                   RHEOSTAT_TIMEOUT);
  else
    retcode = i2c_read_timeout_us(I2C_DEV, addr, buffer, len, false,
                                  RHEOSTAT_TIMEOUT);

  if (retcode == PICO_ERROR_TIMEOUT) {
    // reinitialize I2C peripheral to put it into a consistent state
    i2c_init(I2C_DEV, I2C_SPD);
    return PICO_ERROR_TIMEOUT;
  } else if (retcode < 0) {
    // Pass through other errors
    return retcode;
  } else if (retcode != len) {
    // Some bits were lost in transit
    return PICO_ERROR_IO;
  } else {
    // Return PICO_OK if everything checks out
    return PICO_OK;
  }
}

int get_rheostat_position(uint8_t addr) {
  uint8_t tx[] = {0x08, 0x00}; // Read RDAC register
  uint8_t rx[2];

  int retcode;

  // Ask for contents of RDAC
  retcode = wrap_i2c(addr, tx, 2, true);
  if (retcode < 0)
    return retcode;

  retcode = wrap_i2c(addr, rx, 2, false);
  if (retcode < 0)
    return retcode;

  // Transform the result into a number
  // The rheostat returns it in this format:
  // 000000dd dddddd00
  int result = (rx[0] << 6) | (rx[1] >> 2);
  return result;
}

int set_rheostat_position(uint8_t addr, int value) {
  // Do some bounds checking
  if (value >= (1 << 8) || value < 0)
    return PICO_ERROR_INVALID_DATA;

  uint16_t cmd = (1 << 10) | (value << 2);
  uint8_t tx[] = {
      // Disable write protect
      0x1C,
      0x02,
      // Set wiper position
      (uint8_t)(cmd >> 8),
      (uint8_t)(cmd & 0x00FF),
  };

  int retcode;

  // Transmit settings
  retcode = wrap_i2c(addr, tx, 4, true);
  if (retcode < 0)
    return retcode;

  // Read back settings
  retcode = get_rheostat_position(addr);
  if (retcode < 0) {
    return retcode;
  } else if (retcode != value) {
    return PICO_ERROR_INVALID_STATE;
  } else {
    return retcode;
  }
}

int ensure_rheostat_state(void) {
  int retcode;
  // Ensure limiter is at the value saved in memory
  if (get_rheostat_position(LIM_ADDR) != lim_state) {
    retcode = set_rheostat_position(LIM_ADDR, lim_state);
    if (retcode < 0) {
      return retcode;
    }
  }

  // Ensure monitor is at the value saved in memory
  if (get_rheostat_position(MON_ADDR) != mon_state) {
    retcode = set_rheostat_position(MON_ADDR, mon_state);
    if (retcode < 0) {
      return retcode;
    }
  }

  return PICO_OK;
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
    mon_state = retcode;
    return state_to_monitor(mon_state);
  }
}

float get_monitor_current(void) {
  int retcode = get_rheostat_position(MON_ADDR);
  return retcode < 0 ? retcode : state_to_monitor(retcode);
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
    lim_state = retcode;
    return state_to_limit(lim_state);
  }
}

float get_current_limit(void) {
  int retcode = get_rheostat_position(LIM_ADDR);
  return retcode < 0 ? retcode : state_to_limit(retcode);
}

// TODO: split return value into status and response entries

void set_current_limit_cmd(char *next_token) {
  float target = atof(next_token);
  float result = set_current_limit(target);
  printf("%f\n", result);
}

void get_current_limit_cmd(void) {
  float result = get_current_limit();
  printf("%f\n", result);
}

void set_monitor_current_cmd(char *next_token) {
  float target = atof(next_token);
  float result = set_monitor_current(target);
  printf("%f\n", result);
}

void get_monitor_current_cmd(void) {
  float result = get_monitor_current();
  printf("%f\n", result);
}

#else

void init_rheostats(void) {}

void setup_i2c() {}

int wrap_i2c(uint8_t addr, uint8_t *buffer, size_t len, bool write) {
  return 0;
};

int get_rheostat_postion(uint8_t addr) { return 0; }

int set_rheostat_position(uint8_t addr, int value) { return 0; }

float state_to_monitor(int state) { return 0.0; }

float state_to_limit(int state) { return 0.0; }

float set_monitor_current(float current_A) { return 0.0; }

float get_monitor_current(void) { return 0.0; }

float set_current_limit(float current_A) { return 0.0; }

float get_current_limit(void) { return 0.0; }

int ensure_rheostat_state(void) { return 0; }

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
