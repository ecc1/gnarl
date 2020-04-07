#include <stdio.h>

#include <nvs_flash.h>

#include "wifi.h"

void app_main(void) {
	ESP_ERROR_CHECK(nvs_flash_init());
	wifi_init();
	char *ip_address = wifi_ip_address();
	if (!ip_address) {
		return;
	}
	printf("IP address: %s\n", ip_address);
}
