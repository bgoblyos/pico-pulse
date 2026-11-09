#pragma once

void init_rheostats(void);

void setup_i2c(void);

int wrap_i2c(uint8_t addr, uint8_t *buffer, size_t len, bool write);

int get_rheostat_position(uint8_t addr);

int set_rheostat_position(uint8_t addr, int value);

int ensure_rheostat_state(void);

float state_to_monitor(int state);

float state_to_limit(int state);

float set_monitor_current(float current_A);

float get_monitor_current(void);

float set_current_limit(float current_A);

float get_current_limit(void);

void set_current_limit_cmd(char* next_token);

void get_current_limit_cmd(void);

void set_monitor_current_cmd(char* next_token);

void get_monitor_current_cmd(void);
