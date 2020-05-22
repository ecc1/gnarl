#include <stdlib.h>

#include "network.h"
#include "papertrail.h"

void app_main(void) {
	ESP_ERROR_CHECK(network_init());
	printf("IP address: %s\n", ip_address());
	papertrail_init();
	fprintf(papertrail, "this is a log message from GNARL!");
}
