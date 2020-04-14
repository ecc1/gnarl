#include <unistd.h>

#include <driver/gpio.h>

#include "module.h"

#define MILLISECONDS 1000

void app_main(void) {
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
	int on_off = 0;
	for (;;) {
		on_off ^= 1;
		gpio_set_level(LED, on_off);
		usleep(500*MILLISECONDS);
	}
}
