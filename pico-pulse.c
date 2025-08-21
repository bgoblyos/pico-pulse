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

// Timing variables
uint32_t cpu_clk;
uint32_t cycles_per_s;
uint32_t cycles_per_ms;
uint32_t cycles_per_us;

// Command processing variables
uint32_t rx_counter = 0;      // Keep track of cursor position in receive buffer
int rx_tmp;                   // Temporary buffer for received character
bool rx_busy = false;         // Indicate whether input processing is in progress
bool cmd_ready= false;        // Indicate whether command ready to be processed

// Buffers
char rx_buf[CMD_BUF_LEN];         // Incoming commands are placed into this buffer
char cmd_buf[CMD_BUF_LEN];        // Commands are copied here for processing
uint32_t pio_buf[PIO_BUF_LEN];    // Pulse data is stored here

// This function is called whenerver there's data on the serial port
void rx_handler(void* ptr) {
    // If another instance of this handler is running, spin until it's done
    while (rx_busy)
        sleep_us(1);
    
    // Make sure another instance won't interrupt
    rx_busy = true;

    // Keep trying until a read is successful
    do {
        rx_tmp = stdio_getchar_timeout_us(1);
    } while (rx_tmp == -2);

    // If character is LF or CR or we ran out of space, terminate string and hand off
    // into another buffer for further processing. The non-zero length check prevents
    // extra line terminaltions, so CRLF doesn't result in a new zero-length string
    if (rx_counter != 0 && (rx_tmp == 10 || rx_tmp == 13 || rx_counter == CMD_BUF_LEN - 1)) {
	rx_buf[rx_counter] = '\0';
	
	if (strlen(rx_buf))
	// If the command is still being processed, spin until it's done before overwriting its buffer
	while (cmd_ready)
	    sleep_us(1);
	
	// Copy string into new buffer
	strcpy(cmd_buf, rx_buf);
	// Indicate that command is ready for processing
        cmd_ready = true;
	// Reset counter
	rx_counter = 0;
    }
    // Only consider printable characters
    else if (isprint(rx_tmp)) {
        rx_buf[rx_counter++] = (char)rx_tmp;
    }
    // Free up lock
    rx_busy = false;
}

void init_pio() {
    // Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pulse_program, &pio, &sm, &offset, BASE_GPIO, N_GPIO, true);
    hard_assert(rc);
    printf("Loaded program at %u on pio %u\n", offset, PIO_NUM(pio));

    // Initialize state machine
    pulse_program_init(pio, sm, offset, BASE_GPIO, N_GPIO);
    
    // clear FIFO
    pio_sm_drain_tx_fifo(pio, sm);

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

void start_dma(uint n) {
    dma_channel_configure(
        dma,
        &dma_conf,
        &pio->txf[sm],
        pio_buf,
        dma_encode_transfer_count(n < PIO_BUF_LEN ? n : PIO_BUF_LEN),
        true
    );
};

int main() {
    setup_default_uart();

    init_pio();
    init_dma();

    // Set up input handler
    stdio_set_chars_available_callback(rx_handler, NULL);

    float freq = 4.0;
    uint32_t delay = (uint32_t)(clock_get_hz(clk_sys) / freq) - 4;
    for (uint i = 0; i < 32; i++)
        pio_buf[i] = ((delay << N_GPIO) | i);

    
    start_dma(32);

    sleep_ms(10000);
    start_dma(32);
    
    // Spin forever
    while (1)
        tight_loop_contents();

}

