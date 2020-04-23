#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define TAG		"NS"

#include <esp_log.h>

#include <cJSON.h>

#include "nightscout.h"

struct timeval timeval_from_milliseconds(double ms) {
	struct timeval tv = {
		.tv_sec = ms / 1000,
		.tv_usec = fmod(ms, 1000) * 1000,
	};
	return tv;
}

static void do_entry(const cJSON *e, nightscout_entry_callback_t cb) {
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
		ESP_LOGE(TAG, "JSON entry has no date field");
		return;
	}
	struct timeval tv = timeval_from_milliseconds(item->valuedouble);

	item = cJSON_GetObjectItem(e, "sgv");
	if (!item) {
		ESP_LOGI(TAG, "ignoring JSON entry with no sgv field");
		return;
	}
	int sgv = item->valueint;

	nightscout_entry_t entry = {
		.tv = tv,
		.sgv = sgv,
	};
	cb(&entry);
}

void process_nightscout_entries(const char *json, nightscout_entry_callback_t callback) {
	if (!json) {
		ESP_LOGE(TAG, "no response");
		return;
	}
	cJSON *root = cJSON_Parse(json);
	if (!root || !cJSON_IsArray(root)) {
		ESP_LOGE(TAG, "response \"%s\" is not a JSON array", json);
		return;
	}
	int n = cJSON_GetArraySize(root);
	ESP_LOGI(TAG, "received JSON array of %d entries", n);
	for (int i = 0; i < n; i++) {
		do_entry(cJSON_GetArrayItem(root, i), callback);
	}
	cJSON_Delete(root);
}

void print_nightscout_entry(const nightscout_entry_t *e) {
	printf("%s  %3d\n", nightscout_time_string(e->tv.tv_sec), e->sgv);
}
