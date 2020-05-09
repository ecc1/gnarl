#include <strings.h>
#include <time.h>

#define TAG		"HTTP"

#include <esp_log.h>
#include <esp_http_client.h>

#include "nightscout.h"

time_t http_server_time;

char *http_get(esp_http_client_handle_t client) {
	http_server_time = 0;
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_http_client_open: %s", esp_err_to_name(err));
		return 0;
	}
	int content_length = esp_http_client_fetch_headers(client);
	static char response[8192];
	char *p = response;
	int len = content_length;
	if (len == -1) {
		ESP_LOGE(TAG, "esp_http_client_fetch_headers: failure");
		return 0;
	}
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

static const char *rfc1123_format = "%a, %d %b %Y %T %Z";

esp_err_t http_header_callback(esp_http_client_event_t *e) {
	if (e->event_id != HTTP_EVENT_ON_HEADER) {
		return ESP_OK;
	}
	char *key = e->header_key;
	char *value = e->header_value;
	ESP_LOGD(TAG, "HTTP header %s: %s", key, value);
	if (strcasecmp(key, "date") != 0) {
		return ESP_OK;
	}
	struct tm tm;
	char *p = strptime(value, rfc1123_format, &tm);
	if (!p) {
		ESP_LOGE(TAG, "cannot parse \"%s\" in RFC1123 format", value);
		return ESP_OK;
	}
	http_server_time = make_gmt(&tm);
	return ESP_OK;
}
