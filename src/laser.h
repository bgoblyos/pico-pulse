#pragma once

void init_laser(void);

void set_laser_state(bool state);

bool get_laser_error(void);

void set_laser_state_cmd(char* next_token);

void get_laser_state_cmd(void);

void get_laser_error_cmd(void);
