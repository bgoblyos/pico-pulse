// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#include "board_config.h"

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

void set_laser_state(bool state) {
    gpio_put(LASER_NSLP, state);
    laser_state = state;
}

bool get_laser_error(void) {
    return gpio_get(LASER_NERR);
}

void set_laser_state_cmd(char* next_token) {
    int state = atoi(next_token);
    set_laser_state((bool)(state));
    printf("ACK\n");
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

void set_laser_state(bool state) {}

bool get_laser_error(void) {return 0;}

void set_laser_state_cmd(char* next_token) {}

void get_laser_state_cmd() {
	printf("ERR: This firmware was built without laser driver support.\n");
}

void get_laser_error_cmd() {
	printf("ERR: This firmware was built without laser driver support.\n");
}

#endif
