// Copyright (c) 2025 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"

#include "hardware.h"
#include "command.h"
#include "status.h"

// PIO parameters
// Defined here for ease of access
const uint pio_base_gpio = 2;              // Number of first GPIO to be used as output
const uint pio_n_gpio = 5;                 // Number of consecutive GPIOs to use. Don't forget to update the PIO code if you change this!
const uint32_t pio_extra_cycles = 4;       // Number of cycles it takes the PIO to loop if the delay is 0
#define PIO_BUF_LEN 65536                  // PIO instruction buffer length
const uint32_t pio_buf_len = PIO_BUF_LEN;  // Save it to a constant as well for convenience
uint32_t pio_buf[PIO_BUF_LEN];             // Buffer for storing data for the PIO

// Pull in DMA related variables from hardware.c for use here
extern int dma;          // Number of the DMA channel, used for checking if it's busy
extern uint dma_count;   // Number of transfers to be done by the DMA, set after uploading new sequence

// Timing variables
uint32_t cpu_clk;

// Sequence looping control
uint32_t loop = 0;                // Loop count
const uint32_t loop_inf_val = ~0; // Max value of uint32_t, treated as inf

// Pull in command processing flags from command.c
extern bool cmd_ready;          // Indicate whether command ready to be processed
extern bool rx_available;       // Indicate whether a character is avaialble on stdin


int main() {
	// Initialize status LED
	status_init();
	status_on();

    // Initialize serial communication on UART
    setup_default_uart();
	stdio_init_all();
    // Set up input handler
    stdio_set_chars_available_callback(rx_handler, NULL);

    // Initialize PIO and DMA channel
    init_pio();
    init_dma();

    // Get system clock speed
    cpu_clk = clock_get_hz(clk_sys);

	status_off();

    // Main loop
	while (1) {
		// If there are characters available, read one of them in.
		// This might be slower than reading them in until the buffer is emtpy,
		// but the DMA requires frequent attention and somewhat consistent timings
		if (rx_available) {
			cmd_read_char();
		}

		// If a new command has been read in, decode it
		if (cmd_ready) {
			status_on();
			cmd_decode();
			// Indicate that command has been processed
			cmd_ready = false;
			status_off();
		}

		// If DMA looping is requested and DMA is idle, restart it
		if (loop != 0 && !dma_channel_is_busy(dma)) {
			start_dma();
			// If looping is finite, decrement counter
			if (loop != loop_inf_val)
				loop--;
		}
	}

}

