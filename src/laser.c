// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "board_config.h"
#include "laser.h"
#include "rheostat.h"

#define BOOL2STR(b) (b ? "true" : "false")

bool laser_state = false;

#if USE_LASER_DRIVER

void init_laser(void) {
  // Initialize pins
  gpio_init(LASER_NERR);
  gpio_init(LASER_NSLP);
  // Set direction
  gpio_set_dir(LASER_NERR, GPIO_IN);
  gpio_set_dir(LASER_NSLP, GPIO_OUT);

  laser_state = false;
}

int set_laser_state(bool state) {
  // Before turning on the laser, make sure the rheostat is consistent.
  // Abort if consistency cannot be achieved.
  if (state) {
    int retcode = ensure_rheostat_state();
    if (retcode < 0) {
      return retcode;
    }
  }

  gpio_put(LASER_NSLP, state);
  laser_state = state;
  return PICO_OK;
}

bool get_laser_error(void) { return !gpio_get(LASER_NERR); }

void set_laser_state_cmd(char *next_token) {
  int state = atoi(next_token);
  // TODO: Use some custom error table instead of the Pico SDK enum
  int retcode = set_laser_state((bool)(state));
  int error = get_laser_error();
  printf("{\"status\": %d, \"enabled\": %s, \"error_state\": %s}\n", retcode,
         BOOL2STR(laser_state), BOOL2STR(error));
}

void get_laser_state_cmd() {
  int error = get_laser_error();
  printf("{\"status\": 0, \"enabled\": %s, \"error_state\": %s}\n",
         BOOL2STR(laser_state), BOOL2STR(error));
}

#else

void init_laser(void) {}

void set_laser_state(bool state) {}

int set_laser_state_cmd(char *next_token) {}

void get_laser_state_cmd() {
  printf(
      "{\"status\": 1, \"error_msg\": \"This firmware was built without laser "
      "driver support.\"}\n");
}

#endif
