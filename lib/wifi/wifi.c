#define TAG		"WiFi"

#include <esp_log.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include "wifi_config.h"

#define MAX_RETRIES  		5
#define RETRY_INTERVAL		100	// milliseconds
#define IP_TIMEOUT		10000	// milliseconds

static esp_netif_t *wifi_interface;
static int retry_num;
static volatile TaskHandle_t waiting_task;

static void handle_wifi_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	switch (event_id) {
	case WIFI_EVENT_STA_START:
		ESP_LOGD(TAG, "WIFI_EVENT STA_START");
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_CONNECTED:
		ESP_LOGD(TAG, "WIFI_EVENT STA_CONNECTED");
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		ESP_LOGD(TAG, "WIFI_EVENT STA_DISCONNECTED");
		if (arg != 0) {
			wifi_event_sta_disconnected_t *d = arg;
			ESP_LOGD(TAG, "reason = %d", d->reason);
		}
		if (retry_num == MAX_RETRIES) {
			ESP_LOGE(TAG, "failed to connect to %s after %d attempts", WIFI_SSID, MAX_RETRIES);
			return;
		}
		retry_num++;
		vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL));
		esp_wifi_connect();
		break;
	}
}

static void handle_ip_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	switch (event_id) {
	case IP_EVENT_STA_GOT_IP:
		ESP_LOGD(TAG, "IP_EVENT STA_GOT_IP");
		retry_num = 0;
		if (waiting_task) {
			xTaskNotify(waiting_task, 0, 0);
		}
		break;
	default:
		ESP_LOGD(TAG, "unexpected IP_EVENT %d", event_id);
		break;
	}
}

static void wifi_init(void) {
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_interface = esp_netif_create_default_wifi_sta();

	wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, handle_wifi_event, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, handle_ip_event, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	wifi_config_t wifi_cfg = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
	ESP_LOGI(TAG, "connecting to %s", WIFI_SSID);
	ESP_ERROR_CHECK(esp_wifi_start());

	waiting_task = xTaskGetCurrentTaskHandle();
	if (!xTaskNotifyWait(0, 0, 0, pdMS_TO_TICKS(IP_TIMEOUT))) {
		ESP_LOGE(TAG, "timeout waiting for IP address");
	}
}

char *ip_address(void) {
	static char addr[20];
	esp_netif_ip_info_t ip_info;
	ESP_ERROR_CHECK(esp_netif_get_ip_info(wifi_interface, &ip_info));
	esp_ip4addr_ntoa(&ip_info.ip, addr, sizeof(addr));
	return addr;
}

void app_main_with_wifi(void);

void app_main(void) {
	wifi_init();
	ESP_LOGI(TAG, "IP address: %s", ip_address());
	app_main_with_wifi();
	ESP_LOGD(TAG, "Return from main application function");
}
