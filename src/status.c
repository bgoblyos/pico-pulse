#include "status.h"
#include "pico/stdlib.h"

void status_init() {
	// For the time being, this only works with the non-W Pico 2
	// The 2 W would required the wireless chip to be initialized,
	// which might cause some interference
#if defined(PICO_DEFAULT_LED_PIN)
	gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}

void status_on() {
#if defined(PICO_DEFAULT_LED_PIN)
	gpio_put(PICO_DEFAULT_LED_PIN, true);
#endif
}

void status_off() {
#if defined(PICO_DEFAULT_LED_PIN)
	gpio_put(PICO_DEFAULT_LED_PIN, false);
#endif
}
