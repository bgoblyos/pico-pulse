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

#define CMD_BUF_LEN 65536     // Incoming command buffer length
#define PIO_BUF_LEN 65536     // PIO instruction buffer length

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
uint32_t rx_counter = 0;         // Keep track of cursor position in receive buffer
int rx_tmp;                      // Temporary buffer for received character
bool cmd_ready = false;          // Indicate whether command ready to be processed
bool rx_available = false;       // Indicate whether a character is avaialble on stdin

// Buffers
char cmd_buf[CMD_BUF_LEN];        // Commands are copied here for processing
uint32_t pio_buf[PIO_BUF_LEN];    // Pulse data is stored here

void rx_handler(void* ptr) {
	rx_available = true;
}

// This function is called in the main loop every time there's a character available
void cmd_read_char() {
    // Read in single char
    rx_tmp = stdio_getchar_timeout_us(0);

	// If the new character is received successfully, process it
	if (rx_tmp != -2) {
    	// If character is LF or CR or we ran out of space, terminate string and hand off
    	// into another buffer for further processing. The non-zero length check prevents
    	// extra line terminations, so CRLF doesn't result in a new zero-length string
    	if (rx_counter != 0 && (rx_tmp == 10 || rx_tmp == 13 || rx_counter == CMD_BUF_LEN - 1)) {
			// This throws away the last character if the buffer is filled up, but overrunning the buffer would truncate the command regardless,
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
	printf("String: %s\r\n", cmd_buf);
	// Simulate slow decoding
	// sleep_ms(1);
}

int main() {
    // Initialze serial communication on UART
    setup_default_uart();
	stdio_init_all();
    // Set up input handler
    stdio_set_chars_available_callback(rx_handler, NULL);

    // Initilaize PIO and DMA channel
    init_pio();
    init_dma();

    // Get system clock speed
    cpu_clk = clock_get_hz(clk_sys);

	// Test DMA, remove one commands are implemented
	dma_count = PIO_BUF_LEN - 4;
    loop_inf = true;
    float freq = 8.0;
    uint32_t delay = (uint32_t)(clock_get_hz(clk_sys) / freq) - 4;
    for (uint i = 0; i < PIO_BUF_LEN - 3; i++) {
        pio_buf[i++] = ((150-4 << N_GPIO) | 24);
        pio_buf[i++] = ((150000-4 << N_GPIO) | 0);
        pio_buf[i++] = ((150-4 << N_GPIO) | 24);
        pio_buf[i] = ((300000-4 << N_GPIO) | 0);
    }

    pio_buf[0] = 24;
    pio_buf[1] = 0;
    dma_count = 2;
    loop_inf = true;

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
		if ((loop_inf || loop_n > 0) && !dma_channel_is_busy(dma)) {
			start_dma();
			// If looping is finite, decrement counter
			if (!loop_inf)
				loop_n--;
		}
	}

}

