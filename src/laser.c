// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

#include "board_config.h"
#include "laser.h"
#include "rheostat.h"

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
  int retcode = ensure_rheostat_state();
  if (retcode < 0) {
    return retcode;
  }

  gpio_put(LASER_NSLP, state);
  laser_state = state;
  return PICO_OK;
}

bool get_laser_error(void) { return !gpio_get(LASER_NERR); }

void set_laser_state_cmd(char *next_token) {
  int state = atoi(next_token);
  int retcode = set_laser_state((bool)(state));
  if (retcode == PICO_OK) {
    printf("ACK\n");
  } else {
    printf("ERR: Rheostats could not be reset to stored values.\n");
  }
}

void get_laser_state_cmd() {
  int val = laser_state ? 1 : 0;
  printf("%d\n", val);
}

void get_laser_error_cmd() {
  int val = get_laser_error() ? 1 : 0;
  printf("%d\n", val);
}

#else

void init_laser(void) {}

int set_laser_state(bool state) { return 0; }

bool get_laser_error(void) { return 0; }

void set_laser_state_cmd(char *next_token) {}

void get_laser_state_cmd() {
  printf("ERR: This firmware was built without laser driver support.\n");
}

void get_laser_error_cmd() {
  printf("ERR: This firmware was built without laser driver support.\n");
}

#endif
