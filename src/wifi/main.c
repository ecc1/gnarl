#include <stdio.h>

#include "wifi.h"

void app_main(void) {
	wifi_init();
	char *ip_address = wifi_ip_address();
	if (!ip_address) {
		return;
	}
	printf("IP address: %s\n", ip_address);
}
