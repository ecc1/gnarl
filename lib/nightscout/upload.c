#define TAG		"NS"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#include "nightscout.h"

void nightscout_upload(esp_http_client_handle_t client, const char *endpoint, const char *json) {
	ESP_LOGD(TAG, "upload to %s: %s", endpoint, json);
	esp_http_client_set_url(client, endpoint);
	ESP_LOGE(TAG, "upload not implemented yet");
}
