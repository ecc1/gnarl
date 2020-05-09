#define TAG		"NS"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#include "network.h"
#include "nightscout.h"
#include "nightscout_config.h"

esp_err_t http_header_callback(esp_http_client_event_t *e);

#ifdef USE_BLUETOOTH_TETHERING
#define XDRIP_HOST	gateway_address()
#else
#define XDRIP_HOST	XDRIP_WIFI_HOST
#endif

esp_http_client_handle_t xdrip_client_handle(void) {
	static char url[256];
	snprintf(url, sizeof(url), "http://%s:17580/sgv.json", XDRIP_HOST);
	esp_http_client_config_t config = {
		.url = url,
		.timeout_ms = 10000,
		.event_handler = http_header_callback,
	};
	ESP_LOGI(TAG, "xDrip URL: %s", config.url);
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_header(client, "api-secret", NIGHTSCOUT_API_SECRET);
	return client;
}
