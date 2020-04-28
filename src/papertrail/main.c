#include <stdlib.h>
#include <time.h>

#include "network.h"
#include "timezone_config.h"
#include "papertrail.h"

void app_main(void) {
	printf("IP address: %s\n", ip_address());
	setenv("TZ", TZ, 1);
	tzset();

	papertrail_init();
	fprintf(papertrail, "this is a log message from GNARL!");
}
