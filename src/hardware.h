#pragma once

#include "hardware/pio.h"
#include "hardware/dma.h"

void init_pio(void);
void init_dma(void);
void start_dma(void);
void stop_all(void);
uint32_t is_busy(void);
