#include <esp_http_client.h>
#include <lwip/ip_addr.h>
#include <lwip/netdb.h>

#include "tether.h"

static void lookup_host(const char *hostname) {
	struct hostent *he = gethostbyname(hostname);
	if (!he) {
		printf("host %s not found\n", hostname);
		return;
	}
	char *addr = ip4addr_ntoa((ip4_addr_t *)he->h_addr_list[0]);
	printf("%s has address %s\n", hostname, addr);
}

static char *http_get(const char *url) {
	esp_http_client_config_t config = { .url = url };
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		printf("HTTP client connection to %s failed: %s\n", url, esp_err_to_name(err));
		return 0;
	}
	int content_length = esp_http_client_fetch_headers(client);
	static char response[512];
	char *p = response;
	int len = content_length;
	if (len >= sizeof(response)) {
		printf("%d-byte HTTP response is too large for %d-byte buffer\n",
			 content_length, sizeof(response));
		len = sizeof(response)-1;
	}
	while (len > 0) {
		int n = esp_http_client_read(client, p, len);
		if (n <= 0) {
			printf("esp_http_client_read returned %d\n", n);
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

void app_main(void) {
	printf("IP address: %s\n", ip_address());
	printf("Gateway:    %s\n", gateway_address());
	lookup_host("google.com");
	char *resp = http_get("http://postman-echo.com/get?alpha=xyz&beta=1000&gamma=true");
	puts(resp ? resp : "(no response)");
}
