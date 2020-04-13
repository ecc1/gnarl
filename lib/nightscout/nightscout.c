#include "nightscout.h"
#include "nightscout_config.h"

#define NIGHTSCOUT_URL	"https://" NIGHTSCOUT_HOST "/api/v1/entries.json"

// Root cert for Nightscout server.
// The PEM file was extracted from the output of this command:
//   openssl s_client -showcerts -connect NIGHTSCOUT_HOST:443 </dev/null
// The CA root cert is the last cert given in the chain of certs.
//
// To embed it in the app binary, the PEM file is named
// in the component.mk COMPONENT_EMBED_TXTFILES variable.

extern const char root_cert_pem_start[] asm("_binary_root_cert_pem_start");

esp_http_client_handle_t nightscout_client_handle(void) {
	esp_http_client_config_t config = {
		.url = NIGHTSCOUT_URL,
		.cert_pem = root_cert_pem_start,
	};
	return esp_http_client_init(&config);
}
