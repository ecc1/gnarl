#include <unistd.h>

#include <driver/gpio.h>
#include "module.h"

void led_on() {
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
	gpio_set_level(LED, 1);
}

void led_off() {
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
	gpio_set_level(LED, 0);
}


void blink() {
    while(1) {
        led_on();
        for(int i = 0; i < 999999; i++);
        led_off();
        for(int i = 0; i < 999999; i++);
    }
    return 0;
}
