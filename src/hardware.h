#pragma once

#include "hardware/pio.h"
#include "hardware/dma.h"

void init_pio(uint base_gpio, uint n_gpio);
void init_dma(uint pio_buf_len);
void start_dma(void);
void stop_all(void);
