#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <esp_task_wdt.h>

#include "pump_config.h"
#include "rfm95.h"

#define TIMEOUT 60*1000	// 1 minute, in milliseconds

uint8_t rx_buf[256];

void app_main() {
	rfm95_init();
	uint8_t v = read_version();
	printf("radio version %d.%d\n", version_major(v), version_minor(v));
	set_frequency(PUMP_FREQUENCY);
	printf("frequency set to %d Hz\n", read_frequency());
	for (;;) {
		esp_task_wdt_reset();
		int n = sleep_receive(rx_buf, sizeof(rx_buf), TIMEOUT);
		if (n == 0) {
			printf("[timeout]\n");
			continue;
		}
		for (int i = 0; i < n; i++) {
			printf("%02X ", rx_buf[i]);
		}
		printf("(RSSI = %d, # bytes = %d, count = %d)\n", read_rssi(), n, rx_packet_count());
	}
}
