#define TAG		"HTTP"

#include <esp_http_client.h>
#include <esp_log.h>

char *http_get(esp_http_client_handle_t client) {
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "http_get: %s", esp_err_to_name(err));
		return 0;
	}
	int content_length = esp_http_client_fetch_headers(client);
	static char response[8192];
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
