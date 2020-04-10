#include <stdlib.h>
#include <time.h>

#include <nvs_flash.h>

#include "nightscout.h"
#include "nightscout_config.h"
#include "timezone.h"
#include "wifi.h"

static void print_entry(const nightscout_entry_t *e) {
	struct tm *tm = localtime(&e->time);
	static char buf[20];
	strftime(buf, sizeof(buf), "%F %T", tm);
	printf("%s  %3d\n", buf, e->sgv);
}

void app_main(void) {
	ESP_ERROR_CHECK(nvs_flash_init());
	wifi_init();
	setenv("TZ", TZ, 1);
	tzset();
	char *response = http_get(nightscout_client_handle());
	if (!response) {
		return;
	}
	process_nightscout_entries(response, print_entry);
}
