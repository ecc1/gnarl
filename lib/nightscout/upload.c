#include <string.h>

#define TAG		"NS"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#include "nightscout.h"

void nightscout_upload(esp_http_client_handle_t client, const char *endpoint, const char *json) {
	int json_len = strlen(json);
#ifdef NIGHTSCOUT_DEBUG
	ESP_LOGD(TAG, "*** not uploading %d bytes to %s ***", json_len, endpoint);
	printf("%s\n", json);
#else
	ESP_LOGD(TAG, "uploading %d bytes to %s", json_len, endpoint);
	esp_http_client_set_url(client, endpoint);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	// Content-Type must be set before the POST data.
	esp_http_client_set_header(client, "content-type", "application/json");
	esp_http_client_set_header(client, "api-secret", NIGHTSCOUT_API_SECRET);
	esp_http_client_set_post_field(client, json, json_len);
	esp_err_t err = esp_http_client_perform(client);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "upload to %s failed: %s", endpoint, esp_err_to_name(err));
	}
#endif
}
