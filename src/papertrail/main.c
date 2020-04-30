#include <stdlib.h>

#include "network.h"
#include "papertrail.h"

void app_main(void) {
	network_init();
	printf("IP address: %s\n", ip_address());
	papertrail_init();
	fprintf(papertrail, "this is a log message from GNARL!");
}
