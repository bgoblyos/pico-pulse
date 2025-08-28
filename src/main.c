// Copyright (c) 2025 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"

#include "hardware.h"
#include "command.h"

// PIO parameters
// Defined here for ease of access
#define BASE_GPIO 2                  // Start of the GPIO range
#define N_GPIO 5                     // Number of GPIO pins (also change PIO code if you modify this!)
const uint32_t pio_extra_cycles = 4; // Number os cycles it takes the PIO to loop if the delay is 0
#define PIO_BUF_LEN 65536            // PIO instruction buffer length
uint32_t pio_buf[PIO_BUF_LEN];       // Pulse data is stored here


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
    // Initialize serial communication on UART
    setup_default_uart();
	stdio_init_all();
    // Set up input handler
    stdio_set_chars_available_callback(rx_handler, NULL);

    // Initialize PIO and DMA channel
    init_pio(BASE_GPIO, N_GPIO);
    init_dma(PIO_BUF_LEN);

    // Get system clock speed
    cpu_clk = clock_get_hz(clk_sys);

	// Test DMA, remove once commands are implemented
	dma_count = 32;
    loop = loop_inf_val;
    float freq = 8.0;
    uint32_t delay = (uint32_t)(clock_get_hz(clk_sys) / freq) - 4;
    for (uint i = 0; i < 32; i++) {
		pio_buf[i] = (delay << N_GPIO | i);
    }

    start_dma();

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
			cmd_decode();
			// Indicate that command has been processed
			cmd_ready = false;
		}

		// If DMA looping is requested and DMA is idle, restart it
		// TODO: Perhaps this should be implemented as an interrupt handler?
		if (loop != 0 && !dma_channel_is_busy(dma)) {
			start_dma();
			// If looping is finite, decrement counter
			if (loop != loop_inf_val)
				loop--;
		}
	}

}

