#include "network_config.h"

char *ip_address(void);
char *gateway_address(void);

#ifdef USE_BLUETOOTH_TETHERING

void tether_init(void);
#define network_init	tether_init

#else

void wifi_init(void);
#define network_init	wifi_init

#endif