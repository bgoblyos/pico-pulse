#pragma once

void decode_sequence(char* next_token, bool time_in_cycles);
uint32_t parse_entry(char** next_token_ptr, uint32_t* i_ptr, char* err, bool time_in_cycles);
bool attempt_insertion(uint32_t delay, uint32_t output, uint32_t i);
