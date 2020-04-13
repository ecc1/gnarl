#include <stdio.h>

#include <lwip/ip_addr.h>
#include <lwip/netdb.h>

#include "wifi.h"

static void lookup_host(const char *hostname) {
	struct hostent *he = gethostbyname(hostname);
	if (!he) {
		printf("host %s not found\n", hostname);
		return;
	}
	char *addr = ip4addr_ntoa((ip4_addr_t *)he->h_addr_list[0]);
	printf("%s has address %s\n", hostname, addr);
}

void app_main(void) {
	printf("IP address: %s\n", ip_address());
	lookup_host("google.com");
}
