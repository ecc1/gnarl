#include "nightscout.h"
#include "nightscout_config.h"

esp_http_client_handle_t xdrip_client_handle(const char *hostname) {
	static char url[256];
	snprintf(url, sizeof(url), "http://%s:17580/sgv.json", hostname);
	esp_http_client_config_t config = {
		.url = url,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_header(client, "api-secret", NIGHTSCOUT_API_SECRET);
	return client;
}
