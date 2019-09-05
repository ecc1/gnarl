#include <stdio.h>

#include <nvs_flash.h>

#include "wifi.h"

void app_main(void) {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	wifi_init();
	char *ip_address = wifi_ip_address();
	if (!ip_address) {
		return;
	}
	printf("IP address: %s\n", ip_address);
}
