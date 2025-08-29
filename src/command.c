// Copyright (c) 2025 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "command.h"
#include "hardware.h"
#include "version.h"
#include "pulse.h"

// Incoming command buffer
#define CMD_BUF_LEN 65536
char cmd_buf[CMD_BUF_LEN]; 

// Buffer for storing board id
#define BID_BUF_LEN 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1
char bid_buf[BID_BUF_LEN]; 

bool cmd_ready = false;           // Indicate whether command ready to be processed
bool rx_available = false;        // Indicate whether a character is avaialble on stdin

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

// This function is set as the callback when chars are available on stdin
void rx_handler(void* ptr) {
	rx_available = true;
}

// This function is called in the main loop every time there's a character available
void cmd_read_char() {
	static uint32_t rx_counter = 0;  // Keep track of cursor position in receive buffer
	static int rx_tmp;               // Temporary buffer for received character

    // Read in single char
    rx_tmp = stdio_getchar_timeout_us(0);

	// If the new character is received successfully, process it
	if (rx_tmp != -2) {
    	// If character is LF or CR or we ran out of space, terminate string and hand off
    	// into another buffer for further processing. The non-zero length check prevents
    	// extra line terminations, so CRLF doesn't result in a new zero-length string
    	if (rx_counter != 0 && (rx_tmp == 10 || rx_tmp == 13 || rx_counter == CMD_BUF_LEN - 1)) {
			// This throws away the last character if the buffer is filled up,
			// but overrunning the buffer would truncate the command regardless,
			// so it doesn't matter if it happens one character sooner.
			cmd_buf[rx_counter] = '\0';

			//printf("Command read-in successful: %s.\n", cmd_buf);
			// Indicate that command is ready for processing
    		cmd_ready = true;
			// Reset counter
			rx_counter = 0;
    	}
    	// If the received character is printable, convert it to uppercase and append to command buffer
    	else if (isprint(rx_tmp)) {
        	cmd_buf[rx_counter++] = toupper((char)rx_tmp);
			//printf("Read in character %c.\n", (char)rx_tmp);
    	}
	}
	else {
		// If we couldn't get a character, indicate that the buffer is empty
		rx_available = false;
	}
}

// Decode command buffer. Called when the cmd_ready flag is set.
void cmd_decode() {
	// This part isn't performance-critical, so we just use local variables
	char* cmd_word;
	char* next_token;
	
	// Read in first token and store progress in next_token
	cmd_word = strtok_r(cmd_buf, " ", &next_token);

	if (!strcmp(cmd_word, "*IDN?") || !strcmp(cmd_word, "IDN?")) {
		print_id();
	} else if (!strcmp(cmd_word, "CLK?")) {
		print_clk();
	} else if (!strcmp(cmd_word, "BUFFER?")) {
		print_buf();
	} else if (!strcmp(cmd_word, "MAXT?")) {
		print_maxt();
	} else if (!strcmp(cmd_word, "STOP")) {
		stop_all();
		printf("ACK\n"); // Send acknowledgement, since stop_all() is silent
	} else if (!strcmp(cmd_word, "BUSY?")) {
		print_busy();
	} else if (!strcmp(cmd_word, "PULSE")) {
		decode_sequence(next_token, false);
	} else if (!strcmp(cmd_word, "CPULSE")) {
		decode_sequence(next_token, true);
	}
	else {
		printf("Error: command not recognized.\n");
	}
}

void print_id() {
	static bool firstrun = true;

	if (firstrun) {
		// Update ID string on first run
		pico_get_unique_board_id_string(bid_buf, BID_BUF_LEN);
		firstrun = false;
	}

	printf(
		"pico-pulse v%d.%d, board id: %s\n",
		VERSION_MAJOR,
		VERSION_MINOR,
		bid_buf
	);
}

void print_clk() { printf("%lu\n", cpu_clk); }

void print_buf() { printf("%lu\n", pio_buf_len); }

void print_maxt() {
	// Conversion factor from seconds to nanoseconds
	static const uint64_t conv_factor = 1000000000;
	// Absolute maximum delay achieveable with a single pulse
	uint64_t max_cycles = (1 << (32 - pio_n_gpio)) - 1 + pio_extra_cycles;
	// Perform multiplication first to avoid floating point math later on
	uint64_t nanocycles = conv_factor * max_cycles;
	// Perform division with clock rate
	uint64_t maxt_ns = nanocycles / cpu_clk;

	printf("%llu\n", maxt_ns);
}

void print_busy() { printf("%lu\n", is_busy()); }

