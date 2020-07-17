#define TAG		"NS"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#include "nightscout.h"

#define NIGHTSCOUT_BASE	"https://" NIGHTSCOUT_HOST
#define MAX_URL_LEN	256

// Root cert for Nightscout server.
// The PEM file was extracted from the output of this command:
//   openssl s_client -showcerts -connect NIGHTSCOUT_HOST:443 </dev/null
// The CA root cert is the last cert given in the chain of certs.
//
// To embed it in the app binary, the PEM file is named
// in the component.mk COMPONENT_EMBED_TXTFILES variable.

extern const char root_cert_pem_start[] asm("_binary_root_cert_pem_start");

esp_err_t http_header_callback(esp_http_client_event_t *e);

esp_http_client_handle_t nightscout_client_handle(const char *endpoint) {
	static char url[MAX_URL_LEN];
	char *opt_slash = endpoint[0] == '/' ? "" : "/";
	snprintf(url, sizeof(url), "%s%s%s", NIGHTSCOUT_BASE, opt_slash, endpoint);
	esp_http_client_config_t config = {
		.url = url,
		.timeout_ms = 10000,
		.cert_pem = root_cert_pem_start,
		.event_handler = http_header_callback,
	};
	ESP_LOGI(TAG, "Nightscout URL: %s", config.url);
	return esp_http_client_init(&config);
}
