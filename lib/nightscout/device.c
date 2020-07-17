#define TAG		"NS"

#include <esp_log.h>

#include <cJSON.h>

#include "nightscout.h"

static char *device_status_json(nightscout_device_status_t *s) {
	cJSON *root = cJSON_CreateObject();

	char ts[ISO_TIME_STRING_SIZE];
	print_iso_time(ts, s->tv);
	cJSON_AddItemToObject(root, "created_at", cJSON_CreateString(ts));
	cJSON_AddItemToObject(root, "device",  cJSON_CreateString("openaps://GNARL"));

	cJSON *openaps = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "openaps", openaps);
	cJSON *iob = cJSON_CreateObject();
	cJSON_AddItemToObject(openaps, "iob",  iob);
	cJSON *i = cJSON_CreateObject();
	cJSON_AddItemToObject(iob, "iob",  i);
	cJSON_AddItemToObject(i, "iob", cJSON_CreateNumber(s->iob / 1000.0));

	cJSON *pump = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "pump", pump);
	struct timeval tv = { .tv_sec = s->pump_clock };
	print_iso_time(ts, tv);
	cJSON_AddItemToObject(pump, "clock", cJSON_CreateString(ts));
	cJSON_AddItemToObject(pump, "reservoir", cJSON_CreateNumber(s->reservoir / 1000.0));
	cJSON *battery = cJSON_CreateObject();
	cJSON_AddItemToObject(pump, "battery", battery);
	cJSON_AddItemToObject(battery, "voltage",  cJSON_CreateNumber(s->pump_battery / 1000.0));
	cJSON *status = cJSON_CreateObject();
	cJSON_AddItemToObject(pump, "status", status);
	cJSON_AddItemToObject(status, "status",  cJSON_CreateString(s->pump_status));
	cJSON_AddItemToObject(status, "bolusing",  cJSON_CreateBool(s->bolusing));
	cJSON_AddItemToObject(status, "suspended",  cJSON_CreateBool(s->suspended));

	cJSON *uploader = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "uploader", uploader);
	cJSON_AddItemToObject(uploader, "battery",  cJSON_CreateNumber(s->battery_percent));
	cJSON_AddItemToObject(uploader, "batteryVoltage",  cJSON_CreateNumber(s->battery_voltage / 1000.0));

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}

void upload_device_status(esp_http_client_handle_t client, nightscout_device_status_t *s) {
	char *json = device_status_json(s);
	nightscout_upload(client, "api/v1/devicestatus", json);
	free(json);
}
