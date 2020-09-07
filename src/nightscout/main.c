#include <time.h>

#include "network.h"
#include "nightscout.h"
#include "timezone_config.h"

void app_main(void) {
	ESP_ERROR_CHECK(network_init());
	printf("IP address: %s\n", ip_address());
	setenv("TZ", TZ, 1);
	tzset();
	esp_http_client_handle_t ns = nightscout_client_handle();
	char *response = http_get(ns, "/api/v1/entries");
	if (http_server_time) {
		printf("%s  server time\n", nightscout_time_string(http_server_time));
	}
	process_nightscout_entries(response, print_nightscout_entry);
	time_t last = get_last_treatment_time(ns);
	if (last) {
		printf("%s  last treatment\n", nightscout_time_string(last));
	}
	nightscout_client_close(ns);
}
