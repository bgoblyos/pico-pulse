// Copyright (c) 2026 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#define LASER_NERR 19
#define LASER_NSLP 20

bool laser_state = false;

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
