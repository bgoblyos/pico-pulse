#pragma once

void init_rheostats(void);

void setup_i2c(void);

bool reserved_addr(uint8_t addr);

void bus_scan(void);

int set_rheostat_position(uint8_t addr, int value);

float set_monitor_current(float current_A);

float set_current_limit(float current_A);

void set_current_limit_cmd(char* next_token);

void get_current_limit_cmd(void);

void set_monitor_current_cmd(char* next_token);

void get_monitor_current_cmd(void);
