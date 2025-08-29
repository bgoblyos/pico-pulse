#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware.h"
#include "pulse.h"

#define PARSER_SUCCESS 0
#define PARSER_EMPTY 1
#define PARSER_FAILURE 2

// Primes for finding greatest common divisor
#define GCD_PRIMES_LEN 2
const uint64_t primes[GCD_PRIMES_LEN] = {2, 5}; // We only deal with multiples of 10 for now

// Pull in CPU clock rate from main.c
extern uint32_t cpu_clk;

// Pull in PIO variables from main.c
extern uint32_t pio_buf[];
extern const uint32_t pio_buf_len;
extern const uint32_t pio_extra_cycles;
extern const uint pio_n_gpio;

// Pull in DMA variables from main.c
extern uint32_t loop;
extern int dma;
extern uint dma_count;

void decode_sequence(char* next_token, bool time_in_cycles) {
	static char* tmp;
	static char err[256];
	static uint32_t m_target;
	static uint32_t n;

	uint32_t i = 0;

	// Read in m
	tmp = strtok_r(NULL, " ", &next_token);
	if (tmp) {
		m_target = strtoul(tmp, NULL, 10);
	}
	else {
		printf("Error: m parameter could not be parsed.\n");
		loop = 0;
		dma_count = 0;
		return;
	}

	// Read in n
	tmp = strtok_r(NULL, " ", &next_token);
	if (tmp) {
		n = strtoul(tmp, NULL, 10);
	}
	else {
		printf("Error: n parameter could not be parsed.\n");
		loop = 0;
		dma_count = 0;
		return;
	}

	// If we don't intent to start immediately and the DMA is empty,
	// we don't need to abort the current run
	if (!(n == 0 && is_busy() != 2)) {
		stop_all();
	}
	
	bool keepgoing = true;
	while(keepgoing) {
		switch (parse_entry(&next_token, &i, err, time_in_cycles)) {
		case PARSER_EMPTY:
			keepgoing = false;
			break;
		case PARSER_FAILURE:
			printf("Error: %s\n", err);
			loop = 0;
			dma_count = 0;
			return;
		default:
			break;
		}
	}

	for (uint32_t j = 0; j < i; j++)
		printf("%lu,", pio_buf[j]);
	printf(" counter is %lu\n", i);

	loop = n;
	dma_count = i; // Change this once inner loops are implemented
}

uint32_t parse_entry(char** next_token_ptr, uint32_t* i_ptr, char* err, bool time_in_cycles) {
	static char* tmp;
	static uint32_t out;
	static uint64_t time;
	static uint64_t delay;
	static uint64_t simplify;
	static const uint64_t s_to_ns = 1000000000;
	static uint32_t full_pulses;
	static uint32_t remainder;
	uint64_t max_cycles = (1 << (32 - pio_n_gpio)) - 1 + pio_extra_cycles;

	// Read time from parameter list
	tmp = strtok_r(NULL, ",", next_token_ptr);
	if (!tmp)
		return PARSER_EMPTY;

	time = strtoull(tmp, NULL, 10);

	tmp = strtok_r(NULL, ",", next_token_ptr);
	if (!tmp) {
		strcpy(err, "Time entry has no corresponding output mask!");
		return PARSER_FAILURE;
	}
	
	out = strtoul(tmp, NULL, 10);
	if (out >= (1 << pio_n_gpio)) {
		strcpy(err, "Output mask is invalid!");
		return PARSER_FAILURE;
	}

	if (time_in_cycles) {
		delay = time;
	}
	else {
		simplify = gcd(cpu_clk, s_to_ns); // Find simplification factor for cpu_clk/s_to_ns
		if (time > (~(uint64_t)0 / (cpu_clk / simplify))) {
			strcpy(err, "Time is too long to process! Consider using cycle timings instead.");
			return PARSER_FAILURE;
		}
		delay = time * (cpu_clk / simplify) / (s_to_ns / simplify); // Convert ns to cycles
	}
	delay = delay > pio_extra_cycles ? delay : pio_extra_cycles;

	full_pulses = delay / max_cycles;
	remainder = delay % max_cycles;
	
	if (full_pulses != 0)
		for (uint32_t j = 0; j < delay / max_cycles; j++)
			if (!attempt_insertion(max_cycles, out, (*i_ptr)++)) {
				strcpy(err, "Insertion failed, buffer has been overrun.");
				return PARSER_FAILURE;
			}

	if (remainder != 0)
		if (!attempt_insertion(remainder, out, (*i_ptr)++)) {
			strcpy(err, "Insertion failed, buffer has been overrun.");
			return PARSER_FAILURE;
		}

	return PARSER_SUCCESS;

}

bool attempt_insertion(uint32_t delay, uint32_t output, uint32_t i) {
	static uint32_t val;

	if (i >= pio_buf_len)
		return false;

	// Round up delay to at least pio_extra_cycles
	delay = delay > pio_extra_cycles ? delay : pio_extra_cycles;

	// Calculate PIO command value
	val = ((delay - pio_extra_cycles) << pio_n_gpio) | output;

	pio_buf[i] = val;
	return true;
}

uint64_t gcd(uint64_t a, uint64_t b) {
	// Cache previous entry, as this function is expected to be called
	// multiple times with the same arguments
	static uint64_t a_cache = 0;
	static uint64_t b_cache = 0;
	static uint64_t r_cache = 1;

	// Return cached result if possible
	if (a == a_cache && b == b_cache) {
		return r_cache;
	} else {
		a_cache = a;
		b_cache = b;
	}

	uint64_t r = 1;
	bool keepgoing = true;

	while (keepgoing) {
		keepgoing = false;
		for (uint32_t i = 0; i < GCD_PRIMES_LEN; i++) {
			if ((a % primes[i]) == 0 && (b % primes[i]) == 0) {
				r *= primes[i];
				a /= primes[i];
				b /= primes[i];
				keepgoing = true;
			}
		}
	}

	r_cache = r;
	return r;
}
