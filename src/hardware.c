#include "hardware.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico-pulse.pio.h"

// Pull in global buffer and constant
extern uint32_t pio_buf[];
extern uint32_t pio_extra_cycles;

// PIO global variables
PIO pio;
uint sm;
uint offset;

// DMA global variables
int dma;
dma_channel_config dma_conf;
uint dma_count = 0;

// Store some data that was #defined in main
uint32_t pio_max_inst = 0;
uint32_t pio_num_chan = 0;

// DMA looping flags, mostly used in main but required for stop_all
extern bool loop_inf;
extern uint32_t loop_n;

void init_pio(uint base_gpio, uint n_gpio) {
	// Store number of channels for late use
	pio_num_chan = n_gpio;

	// Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
		&pulse_program,
		&pio,
		&sm,
		&offset,
		base_gpio,
		n_gpio,
		true
	);
    hard_assert(rc);

    // Initialize state machine
    pulse_program_init(pio, sm, offset, base_gpio, n_gpio);
    
    // clear FIFO
    pio_sm_clear_fifos(pio, sm);

    // enable state machine
    pio_sm_set_enabled(pio, sm, true);
}

void init_dma(uint pio_buf_len) {
	// Store PIO buffer length
	pio_max_inst = pio_buf_len;
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
        dma_encode_transfer_count(dma_count < pio_max_inst ? dma_count : pio_max_inst),
        true
    );
};

// Stop DMA and flush PIO FIFO
void stop_all() {
	// Turn off infinite DMA looping and set loop count to 0
	loop_inf = false;
	loop_n = 0;
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

