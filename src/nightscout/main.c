#include <time.h>

#include "nightscout.h"
#include "timezone_config.h"

#ifdef USE_WIFI
#include "wifi.h"
#else
#include "tether.h"
#endif

void app_main(void) {
	printf("IP address: %s\n", ip_address());
	setenv("TZ", TZ, 1);
	tzset();
	char *response = http_get(nightscout_client_handle());
	process_nightscout_entries(response, print_nightscout_entry);
}
