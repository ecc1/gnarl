#define TAG		"main"
#define LOG_LOCAL_LEVEL	ESP_LOG_INFO
#include <esp_log.h>

#include <esp_http_client.h>
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
		printf("IP address: %s\n", ip4addr_ntoa(&netif->ip_addr.u_addr.ip4));
		printf("gateway:    %s\n", ip4addr_ntoa(&netif->gw.u_addr.ip4));
	}
}

#define MSECS(n)	((n) / portTICK_PERIOD_MS)
#define SECONDS(n)	MSECS((n) * 1000)

static void wait_for_dhcp(void) {
	int n = 0;
	while (!have_ip_address) {
		link_callback(&bnep_netif);
		status_callback(&bnep_netif);
		if (n++ % 10 == 0) {
			ESP_LOGD(TAG, "waiting for IP address");
		}
		vTaskDelay(MSECS(250));
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

static char *http_get(const char *url) {
	esp_http_client_config_t config = {
		.url = url,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "cannot create HTTP client connection to %s", url);
		return 0;
	}
	int content_length = esp_http_client_fetch_headers(client);
	static char response[1024];
	char *p = response;
	int len = content_length;
	if (len >= sizeof(response)) {
		ESP_LOGE(TAG, "%d-byte HTTP response is too large for %d-byte buffer",
			 content_length, sizeof(response));
		len = sizeof(response)-1;
	}
	while (len > 0) {
		int n = esp_http_client_read(client, p, len);
		if (n <= 0) {
			ESP_LOGE(TAG, "esp_http_client_read returned %d", n);
			err = ESP_FAIL;
			break;
		}
		p += n;
		len -= n;
        }
	*p = 0;
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return err == ESP_OK ? response : 0;
}

void main_loop(void *unused) {
	wait_for_dhcp();
	lookup_host("google.com");
	char *resp = http_get("http://postman-echo.com/get?alpha=xyz&beta=1000&gamma=true");
	if (resp) {
		puts(resp);
	}
	for (;;) {
		vTaskDelay(SECONDS(60));
	}
}
