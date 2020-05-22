#include <time.h>

#include "network.h"
#include "nightscout.h"
#include "timezone_config.h"

void app_main(void) {
	ESP_ERROR_CHECK(network_init());
	printf("IP address: %s\n", ip_address());
	setenv("TZ", TZ, 1);
	tzset();
	char *response = http_get(xdrip_client_handle());
	if (http_server_time) {
		printf("%s  server time\n", nightscout_time_string(http_server_time));
	}
	process_nightscout_entries(response, print_nightscout_entry);
}
