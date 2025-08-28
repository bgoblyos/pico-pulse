// Copyright (c) 2025 Bence Göblyös
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hardware.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico-pulse.pio.h"

// Pull in PIO related constants from main.c
extern uint32_t pio_buf[];
extern const uint32_t pio_buf_len;
extern const uint32_t pio_extra_cycles;
extern const uint pio_base_gpio;
extern const uint pio_n_gpio;

// PIO global variables
PIO pio;
uint sm;
uint offset;

// DMA global variables
int dma;
dma_channel_config dma_conf;
uint dma_count = 0;

// Store some data that was #defined in main

// DMA looping flags, mostly used in main but required for stop_all
extern uint32_t loop;
extern const uint32_t loop_inf_val;

void init_pio() {
	// Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
		&pulse_program,
		&pio,
		&sm,
		&offset,
		pio_base_gpio,
		pio_n_gpio,
		true
	);
    hard_assert(rc);

    // Initialize state machine
    pulse_program_init(pio, sm, offset, pio_base_gpio, pio_n_gpio);
    
    // clear FIFO
    pio_sm_clear_fifos(pio, sm);

    // enable state machine
    pio_sm_set_enabled(pio, sm, true);
}

void init_dma() {
    // Claim DMA channel
    dma = dma_claim_unused_channel(true);
    // Get default config
    dma_conf = dma_channel_get_default_config(dma);
    // Set DMA width to 32 bits
    channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_32);
    // Increment read address
    channel_config_set_read_increment(&dma_conf, true);
    // Do not increment write address
    channel_config_set_write_increment(&dma_conf, false);
    // Connect to FIFO Tx request signals
    channel_config_set_dreq(&dma_conf, pio_get_dreq(pio, sm, true));
}

void start_dma() {
    dma_channel_configure(
        dma,
        &dma_conf,
        &pio->txf[sm],
        pio_buf,
        dma_encode_transfer_count(dma_count < pio_buf_len ? dma_count : pio_buf_len),
        true
    );
};

// Stop DMA and flush PIO FIFO
void stop_all() {
	// Turn off infinite DMA looping and set loop count to 0
	loop = 0;
    // Disable state machine
	pio_sm_set_enabled(pio, sm, true);
	// Stop DMA
	dma_channel_abort(dma);
	// Clear FIFO
    pio_sm_clear_fifos(pio, sm);
    // Re-enable state machine
    pio_sm_set_enabled(pio, sm, true);
	// Send a zero to the PIO to disable all outputs
	pio_sm_put_blocking(pio, sm, 0);
}

uint32_t is_busy() {
	if (loop != 0 || dma_channel_is_busy(dma)) {
		return 2; // DMA is busy
	} else if (!pio_sm_is_tx_fifo_empty(pio, sm)) {
		return 1; // PIO is busy but DMA is idle
	} else {
		return 0; // Nothing is busy
	}
}
