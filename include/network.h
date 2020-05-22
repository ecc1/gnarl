#include "network_config.h"

char *ip_address(void);
char *gateway_address(void);

#ifdef USE_BLUETOOTH_TETHERING

#define NETWORK_CONFIG	"Bluetooth"

int tether_init(void);
#define network_init	tether_init

void tether_off(void);
#define network_off	tether_off

#else

#define NETWORK_CONFIG	"WiFi"

int wifi_init(void);
#define network_init	wifi_init

void wifi_off(void);
#define network_off	wifi_off

#endif
