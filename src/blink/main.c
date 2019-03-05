#include <unistd.h>

#include <driver/gpio.h>

#define LED GPIO_NUM_2 // TTGO LoRa OLED v1 module

#define MILLISECONDS 1000

void app_main() {
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
	int on_off = 0;
	for (;;) {
		on_off ^= 1;
		gpio_set_level(LED, on_off);
		usleep(500*MILLISECONDS);
	}
}
