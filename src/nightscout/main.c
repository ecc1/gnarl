#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nvs_flash.h>

#include <cJSON.h>

#define TAG		"NS"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>
#include <esp_tls.h>

#include "nightscout_config.h"
#include "timezone.h"
#include "wifi.h"

#define NIGHTSCOUT_URL	"https://" NIGHTSCOUT_HOST
#define NS_URL		NIGHTSCOUT_URL "/api/v1/entries.json"

static const char *request = "GET " NS_URL " HTTP/1.0\r\n"
    "Host: "NIGHTSCOUT_HOST"\r\n"
    "User-Agent: esp-idf/4.1 esp32\r\n"
    "\r\n";

// Root cert for Nightscout server.
// The PEM file was extracted from the output of this command:
//   openssl s_client -showcerts -connect NIGHTSCOUT_HOST:443 </dev/null
// The CA root cert is the last cert given in the chain of certs.
//
// To embed it in the app binary, the PEM file is named
// in the component.mk COMPONENT_EMBED_TXTFILES variable.

extern const uint8_t root_cert_pem_start[] asm("_binary_root_cert_pem_start");
extern const uint8_t root_cert_pem_end[]   asm("_binary_root_cert_pem_end");

static const char *https_get() {
	esp_tls_cfg_t cfg = {
		.cacert_buf  = root_cert_pem_start,
		.cacert_bytes = root_cert_pem_end - root_cert_pem_start,
	};
	struct esp_tls *tls = esp_tls_conn_http_new(NS_URL, &cfg);
	if (tls == 0) {
		ESP_LOGE(TAG, "connection to %s failed", NIGHTSCOUT_URL);
		return 0;
	}
	ESP_LOGI(TAG, "connected to %s", NIGHTSCOUT_URL);

	char *p = (char *)request;
	int len = strlen(request);
	while (len > 0) {
		int n = esp_tls_conn_write(tls, p, len);
		if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}
		if (n < 0) {
			ESP_LOGE(TAG, "esp_tls_conn_write returned %d", n);
			esp_tls_conn_delete(tls);
			return 0;
		}
		if (n == 0) {
			ESP_LOGE(TAG, "no response (connection closed)");
			esp_tls_conn_delete(tls);
			return 0;
		}
		ESP_LOGI(TAG, "%d bytes written", n);
		p += n;
		len -= n;
	}

	static char response[4096];
	p = response;
	len = sizeof(response)-1;
	while (len > 0) {
		int n = esp_tls_conn_read(tls, p, len);
		if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}
		if (n < 0) {
			ESP_LOGE(TAG, "esp_tls_conn_read returned %d", n);
			esp_tls_conn_delete(tls);
			return 0;
		}
		if (n == 0) {
			ESP_LOGI(TAG, "connection closed");
			break;
		}
		ESP_LOGI(TAG, "%d bytes read", n);
		p += n;
		len -= n;
	}
	if (len == 0) {
		// Response buffer may be too small!
		ESP_LOGW(TAG, "%d-byte response from %s", sizeof(response)-1, NS_URL);
	}
	*p = 0;
	esp_tls_conn_delete(tls);
	return response;
}

// Find the body after the \r\n\r\n separator.
const char *find_body(const char *response) {
	for (const char *p = response; *p; p++) {
		if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') {
			return &p[4];
		}
	}
	return 0;
}

const cJSON *parse_response(const char *response) {
	const char *body = find_body(response);
	if (!body) {
		return 0;
	}
	return cJSON_Parse(body);
}

void print_entry(const cJSON *e) {
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
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	wifi_init();
	ESP_LOGI(TAG, "setting TZ = %s", TZ);
	setenv("TZ", TZ, 1);
	tzset();
	const char *resp = https_get();
	if (!resp) {
		return;
	}
	const cJSON *root = parse_response(resp);
	if (!root || !cJSON_IsArray(root)) {
		ESP_LOGE(TAG, "expected JSON array from %s", NS_URL);
		puts(resp);
		return;
	}
	int n = cJSON_GetArraySize(root);
	ESP_LOGI(TAG, "received JSON array of %d entries", n);
	for (int i = 0; i < n; i++) {
		print_entry(cJSON_GetArrayItem(root, i));
	}
	cJSON_Delete(root);
}
