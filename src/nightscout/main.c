#include <time.h>

#include "nightscout.h"
#include "nightscout_config.h"
#include "timezone.h"

static void get_entries(void) {
	setenv("TZ", TZ, 1);
	tzset();
	char *response = http_get(nightscout_client_handle());
	if (!response) {
		return;
	}
	process_nightscout_entries(response, print_nightscout_entry);
}

#ifdef USE_WIFI

#include "wifi.h"

void app_main(void) {
	wifi_init();
	get_entries();
}

#else

void app_main_with_tethering(void) {
	get_entries();
}

#endif
