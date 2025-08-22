/*
* Sources:
* pico-examples/pio/pio_blink
* https://gregchadwick.co.uk/blog/playing-with-the-pico-pt4/
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "pico-pulse.pio.h"

#define BASE_GPIO 2           // Start of the GPIO range
#define N_GPIO 5              // Number of GPIO pins (also change PIO code if you modify this!)
#define PIO_EXTRA_CYCLES 4    // Extra cycles taken by PIO code when outputting pulse, used to correct delay

#define CMD_BUF_LEN 1024      // Incoming command buffer length
#define PIO_BUF_LEN 1024      // PIO instruction buffer length

// PIO variables
PIO pio;
uint sm;
uint offset;

// DMA variables
int dma;
dma_channel_config dma_conf;
uint dma_count = 0;

// Timing variables
uint32_t cpu_clk;

// Sequence looping control
bool loop_inf = false;
uint32_t loop_n = 0;

// Command processing variables
uint32_t rx_counter = 0;      // Keep track of cursor position in receive buffer
int rx_tmp;                   // Temporary buffer for received character
bool rx_lock = false;         // Indicate whether input processing is in progress
bool cmd_lock = false;        // Indicate whether command ready to be processed

// Buffers
char rx_buf[CMD_BUF_LEN];         // Incoming commands are placed into this buffer
char cmd_buf[CMD_BUF_LEN];        // Commands are copied here for processing
uint32_t pio_buf[PIO_BUF_LEN];    // Pulse data is stored here

// This function is called whenerver there's data on the serial port
void rx_handler(void* ptr) {
    // If another instance of this handler is running, spin until it's done
    while (rx_lock)
        sleep_us(1);
    
    // Make sure another instance won't interrupt
    rx_lock = true;

    // Keep trying until a read is successful
    do {
        rx_tmp = stdio_getchar_timeout_us(1);
    } while (rx_tmp == -2);

    // If character is LF or CR or we ran out of space, terminate string and hand off
    // into another buffer for further processing. The non-zero length check prevents
    // extra line terminaltions, so CRLF doesn't result in a new zero-length string
    if (rx_counter != 0 && (rx_tmp == 10 || rx_tmp == 13 || rx_counter == CMD_BUF_LEN - 1)) {
		// This throws away the last character if the buffer is filled up, but overrunning the buffer would truncate the command regardless,
		// so it doesn't matter if it happens one character sooner.
		rx_buf[rx_counter] = '\0';
	
		// If the command is still being processed, spin until it's done before overwriting its buffer
		while (cmd_lock)
	    	sleep_us(1);
	
		// Copy string into new buffer
		// Since we already know the length and need to shuffle the whole thing around,
		// we can take the opportunity of uppercase all letters for further use.
		for (uint32_t i = 0; i <= rx_counter; i++)
			cmd_buf[i] = toupper(rx_buf[i]);

		// Indicate that command is ready for processing
    	cmd_lock = true;
		// Reset counter
		rx_counter = 0;
    }
    // Only consider printable characters
    else if (isprint(rx_tmp)) {
        rx_buf[rx_counter++] = (char)rx_tmp;
    }
    // Free up lock
    rx_lock = false;
}

void init_pio() {
    // Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pulse_program, &pio, &sm, &offset, BASE_GPIO, N_GPIO, true);
    hard_assert(rc);
    printf("Loaded program at %u on pio %u\n", offset, PIO_NUM(pio));

    // Initialize state machine
    pulse_program_init(pio, sm, offset, BASE_GPIO, N_GPIO);
    
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
        dma_encode_transfer_count(dma_count < PIO_BUF_LEN ? dma_count : PIO_BUF_LEN),
        true
    );
};

// Stop DMA and flush PIO FIFO
void stop_all() {
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

void cmd_decode() {
	// Decode command from cmd_buf
	printf("CMD decoder called\n");
	printf("String: %s\n", cmd_buf);
	// Release lock before returning
	cmd_lock = false;
}

int main() {
    // Initialze serial communication on UART
    setup_default_uart();
    // Set up input handler
    stdio_set_chars_available_callback(rx_handler, NULL);

    // Initilaize PIO and DMA channel
    init_pio();
    init_dma();

    // Get system clock speed
    cpu_clk = clock_get_hz(clk_sys);

	// Test DMA, remove one commands are implemented
	dma_count = 32;
	loop_inf = false;
    float freq = 16.0;
    uint32_t delay = (uint32_t)(clock_get_hz(clk_sys) / freq) - 4;
    for (uint i = 0; i < 32; i++)
        pio_buf[i] = ((delay << N_GPIO) | i);

    start_dma();

    // Main loop
	while (1) {
		// Slow the main loop down a bit. The command handling doesn't work if the loop is permanently busy (not sure why, exactly)
		sleep_us(1);

		// If there's a new command, decode it
		//printf("CMD lock is %d.\n", cmd_lock);
		if (cmd_lock) {
			printf("Decode logic entered.\n");
			cmd_decode();
		}

		// If DMA looping is requested and DMA is idle, restart it
		// TODO: Perhaps this should be implemented as an interrupt handler?
		if ((loop_inf || loop_n > 0) && !dma_channel_is_busy(dma)) {
			start_dma();
			// If looping is finite, decrement counter
			if (!loop_inf)
				loop_n--;
		}
	}

}

