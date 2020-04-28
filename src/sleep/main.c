#include <stdio.h>
#include <unistd.h>

#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp32/rom/uart.h>

#include "module.h"

static void print_wakeup_reason(void) {
	esp_sleep_wakeup_cause_t r = esp_sleep_get_wakeup_cause();
	const char *reason;
	switch (r) {
	case ESP_SLEEP_WAKEUP_EXT0:
		reason = "external signal using RTC_IO";
		break;
	case ESP_SLEEP_WAKEUP_EXT1:
		reason = "external signal using RTC controller";
		break;
	case ESP_SLEEP_WAKEUP_TIMER:
		reason = "timer";
		break;
	case ESP_SLEEP_WAKEUP_TOUCHPAD:
		reason = "touchpad";
		break;
	case ESP_SLEEP_WAKEUP_ULP:
		reason = "ULP program";
		break;
	default:
		reason = "???";
		break;
	}
	printf("wakeup reason: %s\n", reason);
}

void app_main(void) {
	gpio_set_direction(LED, GPIO_MODE_OUTPUT);
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	esp_sleep_enable_ext0_wakeup(BUTTON, 0);
	for (;;) {
		gpio_set_level(LED, 1);
		sleep(1);
		gpio_set_level(LED, 0);

		uart_tx_wait_idle(0);
		// Wake up on button press or 5-second timeout.
		esp_sleep_enable_timer_wakeup(5000000);
		esp_light_sleep_start();
		print_wakeup_reason();
	}
}
