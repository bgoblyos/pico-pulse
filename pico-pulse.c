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

#define BASE_GPIO 2           // Start of the GPIO range
#define N_GPIO 5              // Number of GPIO pins (also change PIO code if you modify this!)
#define PIO_EXTRA_CYCLES 4    // Extra cycles taken by PIO code when outputting pulse, used to correct delay

void start_dma(int dma, dma_channel_config* conf_ptr, PIO pio, uint sm, uint32_t* buf, uint n) {
    dma_channel_configure(
        dma,
        conf_ptr,
        &pio->txf[sm],
        buf,
        dma_encode_transfer_count(n),
        true
    );
};

int main() {
    setup_default_uart();

    PIO pio;
    uint sm;
    uint offset;

    // Find a free pio and state machine and add the program
    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pulse_program, &pio, &sm, &offset, BASE_GPIO, N_GPIO, true);
    hard_assert(rc);
    printf("Loaded program at %u on pio %u\n", offset, PIO_NUM(pio));

    printf("Clock frequency is %lu HZ\n", clock_get_hz(clk_sys));
    // Initialize state machine
    pulse_program_init(pio, sm, offset, BASE_GPIO, N_GPIO);
    
    // clear FIFO
    pio_sm_drain_tx_fifo(pio, sm);

    // enable state machine
    pio_sm_set_enabled(pio, sm, true);

    float freq = 4.0;
    uint32_t delay = (uint32_t)(clock_get_hz(clk_sys) / freq) - 4;
    uint32_t buf[32];
    for (uint i = 0; i < 32; i++)
        buf[i] = ((delay << N_GPIO) | i);

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
    // Connect to FIFO Tx request signals
    channel_config_set_dreq(&dma_conf, pio_get_dreq(pio, sm, true));
    
    start_dma(dma, &dma_conf, pio, sm, buf, 32);

    sleep_ms(10000);
    start_dma(dma, &dma_conf, pio, sm, buf, 32);
    
    // Spin forever
    while (1)
        tight_loop_contents();

}

