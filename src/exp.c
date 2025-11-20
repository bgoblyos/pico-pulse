#include "exp.h"

#include "hardware.h"

#include "hardware/pio.h"

#define PUSH_PIO(data) pio_sm_put_blocking(pio, sm, data) 

extern PIO pio;
extern uint sm;

void testLoop(void) {
	uint32_t tinit = 10;
	uint32_t tread = 10;
	uint32_t tau = 5;
	uint32_t ref = 100;
	for (uint32_t tau = 0; tau < (uint32_t)(-1); tau++) {
		//PUSH_PIO((tinit << 5) | 1);
		//PUSH_PIO(((ref - tinit) << 5) | 0);
		//PUSH_PIO((tinit << 5) | 1);
		//PUSH_PIO((tau << 5) | 0);
		//PUSH_PIO((tread << 5) | 1);
		//PUSH_PIO(((ref-tinit-tau-tread) << 5) | 0);
		PUSH_PIO((10 << 5) | 1);
		PUSH_PIO((10 << 5) | 0);
	}
}
