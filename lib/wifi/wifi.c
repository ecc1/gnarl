#define TAG		"WiFi"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "wifi.h"

#define MAX_RETRIES  		5
#define RETRY_INTERVAL		100000	// microseconds
#define IP_TIMEOUT		10000	// milliseconds

static int retry_num;

static volatile TaskHandle_t waiting_task;

static esp_netif_ip_info_t ip_info;

char *wifi_ip_address() {
	return ip4addr_ntoa((const ip4_addr_t *)&ip_info.ip);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	if (event_base == WIFI_EVENT) {
		switch (event_id) {
		case WIFI_EVENT_STA_START:
			ESP_LOGD(TAG, "WIFI_EVENT STA_START");
			esp_wifi_connect();
			return;
		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGD(TAG, "WIFI_EVENT STA_CONNECTED");
			return;
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGD(TAG, "WIFI_EVENT STA_DISCONNECTED");
			if (arg != 0) {
				wifi_event_sta_disconnected_t *d = arg;
				ESP_LOGD(TAG, "reason = %d", d->reason);
			}
			if (retry_num == MAX_RETRIES) {
				ESP_LOGI(TAG, "failed to connect to %s after %d attempts", WIFI_SSID, MAX_RETRIES);
				return;
			}
			retry_num++;
			usleep(RETRY_INTERVAL);
			esp_wifi_connect();
			return;
		default:
			ESP_LOGD(TAG, "unexpected WIFI_EVENT %d", event_id);
			return;
		}
	}
	if (event_base == IP_EVENT) {
		switch (event_id) {
		case IP_EVENT_STA_GOT_IP:
			ESP_LOGD(TAG, "IP_EVENT STA_GOT_IP");
			retry_num = 0;
			ip_info = ((ip_event_got_ip_t *)event_data)->ip_info;
			xTaskNotify(waiting_task, 0, 0);
			return;
		default:
			ESP_LOGD(TAG, "unexpected IP_EVENT %d", event_id);
			return;
		}
	}
	ESP_LOGD(TAG, "unexpected event_base = %p", event_base);
}

void wifi_init(void) {
	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_LOGI(TAG, "connecting to %s", WIFI_SSID);
	ESP_ERROR_CHECK(esp_wifi_start());

	waiting_task = xTaskGetCurrentTaskHandle();
	if (!xTaskNotifyWait(0, 0, 0, pdMS_TO_TICKS(IP_TIMEOUT))) {
		ESP_LOGE(TAG, "timeout waiting for IP address");
	}
}
