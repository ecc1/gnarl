#define TAG		"main"
#define LOG_LOCAL_LEVEL	ESP_LOG_INFO
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/dhcp.h>
#include <lwip/netdb.h>
#include <lwip/netif.h>

extern struct netif bnep_netif;

static int dhcp_started = 0;

static void link_callback(struct netif *netif) {
	if (netif_is_link_up(netif) && !dhcp_started) {
		ESP_LOGD(TAG, "link up; starting DHCP");
		dhcp_start(netif);
		dhcp_started = 1;
	} else if (!netif_is_link_up(netif) && dhcp_started) {
		ESP_LOGD(TAG, "link down; stopping DHCP");
		dhcp_stop(&bnep_netif);
		dhcp_started = 0;
	}
}

static volatile int have_ip_address = 0;

static void status_callback(struct netif *netif) {
	if (netif->ip_addr.u_addr.ip4.addr != 0) {
		have_ip_address = 1;
		ESP_LOGI(TAG, "IP address: %s", ip4addr_ntoa(&netif->ip_addr.u_addr.ip4));
	}
}

#define SECONDS(n)	((n) * 1000 / portTICK_PERIOD_MS)

static void wait_for_dhcp(void) {
	int n = 0;
	while (!have_ip_address) {
		link_callback(&bnep_netif);
		status_callback(&bnep_netif);
		if (n++ % 10 == 0) {
			ESP_LOGD(TAG, "waiting for IP address");
		}
		vTaskDelay(SECONDS(1));
	}
}

static void lookup_host(const char *hostname) {
	struct hostent *he = gethostbyname(hostname);
	if (!he) {
		printf("host %s not found\n", hostname);
		return;
	}
	char *addr = he->h_addr_list[0];
	printf("%s has address %s\n", hostname, ip4addr_ntoa((ip4_addr_t *)addr));
}

void main_loop(void *unused) {
	struct netif *netif = &bnep_netif;
	ESP_LOGD(TAG, "BNEP netif addr = %p  name = %c%c  MTU = %d", (void *)netif, netif->name[0], netif->name[1], netif->mtu);
	wait_for_dhcp();
	lookup_host("apple.com");
	lookup_host("espressif.com");
	for (;;) {
		vTaskDelay(SECONDS(60));
	}
}
