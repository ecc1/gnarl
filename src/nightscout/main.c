#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cJSON.h>
#include <nvs_flash.h>

#define TAG		"NS"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>
#include <esp_http_client.h>

#include "nightscout_config.h"
#include "timezone.h"
#include "wifi.h"

#define NIGHTSCOUT_URL	"https://" NIGHTSCOUT_HOST "/api/v1/entries.json"

// Root cert for Nightscout server.
// The PEM file was extracted from the output of this command:
//   openssl s_client -showcerts -connect NIGHTSCOUT_HOST:443 </dev/null
// The CA root cert is the last cert given in the chain of certs.
//
// To embed it in the app binary, the PEM file is named
// in the component.mk COMPONENT_EMBED_TXTFILES variable.

extern const char root_cert_pem_start[] asm("_binary_root_cert_pem_start");

static char response[4096];

static esp_err_t https_get(void) {
	esp_http_client_config_t config = {
		.url = NIGHTSCOUT_URL,
		.cert_pem = root_cert_pem_start,
		.timeout_ms = 10000,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		return err;
	}
	int content_length = esp_http_client_fetch_headers(client);
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
	return err;
}

static void print_entry(const cJSON *e) {
	const cJSON *item = cJSON_GetObjectItem(e, "type");
	if (!item) {
		ESP_LOGE(TAG, "JSON entry has no type field");
		return;
	}
	const char *typ = item->valuestring;
	if (strcmp(typ, "sgv") != 0) {
		ESP_LOGI(TAG, "ignoring JSON entry with type %s", typ);
		return;
	}
	item = cJSON_GetObjectItem(e, "date");
	if (!item) {
		ESP_LOGI(TAG, "JSON entry has no date field");
		return;
	}
	time_t t = (time_t)(item->valuedouble / 1000);
	item = cJSON_GetObjectItem(e, "sgv");
	if (!item) {
		ESP_LOGE(TAG, "ignoring JSON entry with no sgv field");
		return;
	}
	int sgv = item->valueint;
	struct tm *tm = localtime(&t);
	static char buf[20];
	strftime(buf, sizeof(buf), "%F %T", tm);
	printf("%s  %3d\n", buf, sgv);
}

void app_main(void) {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
	wifi_init();
	ESP_LOGI(TAG, "setting TZ = %s", TZ);
	setenv("TZ", TZ, 1);
	tzset();
	err = https_get();
	ESP_ERROR_CHECK(err);
	cJSON *root = cJSON_Parse(response);
	if (!root || !cJSON_IsArray(root)) {
		ESP_LOGE(TAG, "expected JSON array from %s", NIGHTSCOUT_URL);
		ESP_LOGE(TAG, "received: %s\n", response);
		return;
	}
	int n = cJSON_GetArraySize(root);
	ESP_LOGI(TAG, "received JSON array of %d entries", n);
	for (int i = 0; i < n; i++) {
		print_entry(cJSON_GetArrayItem(root, i));
	}
	cJSON_Delete(root);
}
