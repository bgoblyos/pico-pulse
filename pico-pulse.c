/*
* Sources:
* pico-examples/pio/pio_blink
* https://gregchadwick.co.uk/blog/playing-with-the-pico-pt4/
*/

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "pico-pulse.pio.h"

// by default flash leds on gpios 3-4
#ifndef PIO_BLINK_LED1_GPIO
#define PIO_BLINK_LED1_GPIO 3
#endif

int main() {
    setup_default_uart();

    assert(PIO_BLINK_LED1_GPIO < 31);
    assert(PIO_BLINK_LED3_GPIO < 31 || PIO_BLINK_LED3_GPIO >= 32);

    PIO pio;
    uint sm;
    uint offset;

    // Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pulse_program, &pio, &sm, &offset, PIO_BLINK_LED1_GPIO, 2, true);
    hard_assert(rc);
    printf("Loaded program at %u on pio %u\n", offset, PIO_NUM(pio));

    printf("Clock frequency is %lu HZ\n", clock_get_hz(clk_sys));
    // Initialize state machine
    pulse_program_init(pio, sm, offset, PIO_BLINK_LED1_GPIO, 3);
    
    // clear FIFO
    pio_sm_drain_tx_fifo(pio, sm);

    // enable state machine
    pio_sm_set_enabled(pio, sm, true);

    // push values to FIFO
    float freq = 1.0;
    uint32_t delay = (uint32_t)(clock_get_hz(clk_sys) / freq) - 4;
    uint32_t buf[8] = {
        (delay << 3) | 0x0, // Turn on first LED for 1 period
        (delay << 3) | 0x1, // Turn on second LED for 1 period
        (delay << 3) | 0x2, // Turn on both LEDs for 1 period
        (delay << 3) | 0x3, // Turn off all LEDs for 1 period
        (delay << 3) | 0x4, // Turn on first LED for 1 period
        (delay << 3) | 0x5, // Turn on second LED for 1 period
        (delay << 3) | 0x6, // Turn on both LEDs for 1 period
        (delay << 3) | 0x7, // Turn off all LEDs for 1 period
    };

    // Initialize DMA channel
    int dma = dma_claim_unused_channel(true);
    // Get default config
    dma_channel_config dma_conf = dma_channel_get_default_config(dma);
    // Set DMA width to 32 bits
    channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_32);
    // Increment read address
    channel_config_set_read_increment(&dma_conf, true);
    // Do not increment write address
    channel_config_set_write_increment(&dma_conf, false);
    //channel_config_set_write_increment(&dma_conf, true);
    // Set DMA buffer to circular reads with a size of 1 << 4 = 16 bytes (4 elements)
    // channel_config_set_ring(&dma_conf, false, 5);
    // Connect to FIFO Tx request signals
    channel_config_set_dreq(&dma_conf, pio_get_dreq(pio, sm, true));
    // Set up DMA channel
    dma_channel_configure(
        dma,
        &dma_conf,
        &pio->txf[sm], // Write to PIO TX FIFO
        buf, // Read values from buffer
        dma_encode_transfer_count(8), // The buffer has 4 entries, so 16 will go through it 4 times
        true // don't start yet
    );

    sleep_ms(10000);
    dma_channel_configure(
        dma,
        &dma_conf,
        &pio->txf[sm], // Write to PIO TX FIFO
        buf, // Read values from buffer
        dma_encode_transfer_count(8), // The buffer has 4 entries, so 16 will go through it 4 times
        true // don't start yet
    );

    dma_channel_start(dma);
    // Spin forever
    while (1)
        tight_loop_contents();

    printf("Delay is %lu cycles\n", delay);
    for (uint i = 0; i < 8; ++i) {
	//printf("Buffer element at index %u: %lx\n", i, target[i]);
	pio_sm_put_blocking(pio, sm, buf[i]);
    }

    hard_assert(rc);

    // free up pio resources
    pio_sm_unclaim(pio, sm + 1);
    pio_remove_program_and_unclaim_sm(&pulse_program, pio, sm, offset);

    // the program exits but the pio keeps running!
    printf("Program completed\n");
}
