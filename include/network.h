#include "network_config.h"

char *ip_address(void);
char *gateway_address(void);

#ifdef USE_BLUETOOTH_TETHERING

#define app_main	app_main_with_tethering

#else

#define app_main	app_main_with_wifi

#endif
