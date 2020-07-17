#define TAG		"NS"

#include <esp_log.h>

#include <cJSON.h>

#include "nightscout.h"

time_t get_last_treatment_time(void) {
	char *json = http_get(nightscout_client_handle("api/v1/treatments.json?count=1"));
	if (!json) {
		ESP_LOGE(TAG, "no response");
		return 0;
	}
	cJSON *root = cJSON_Parse(json);
	if (!root) {
		ESP_LOGE(TAG, "cannot parse response \"%s\"", json);
		return 0;
	}
	time_t t = 0;
	if (!cJSON_IsArray(root)) {
		ESP_LOGE(TAG, "response \"%s\" is not a JSON array", json);
		goto done;
	}
	int n = cJSON_GetArraySize(root);
	if (n != 1) {
		ESP_LOGE(TAG, "response \"%s\" has length %d, expected 1", json, n);
		goto done;
	}
	const cJSON *item = cJSON_GetObjectItem(cJSON_GetArrayItem(root, 0), "created_at");
	if (!item) {
		ESP_LOGE(TAG, "JSON object has no created_at field");
		goto done;
	}
	t = round_to_seconds(parse_iso_time(item->valuestring));
	if (!t) {
		ESP_LOGE(TAG, "cannot parse ISO time \"%s\"", item->valuestring);
		goto done;
	}
done:
	cJSON_Delete(root);
	return t;
}

static char *treatment_json(nightscout_treatment_t *t) {
	cJSON *root = cJSON_CreateObject();

	char ts[ISO_TIME_STRING_SIZE];
	print_iso_time(ts, t->tv);
	cJSON_AddItemToObject(root, "created_at", cJSON_CreateString(ts));

	cJSON_AddItemToObject(root, "enteredBy",  cJSON_CreateString(NIGHTSCOUT_USER));

	char *e;
	switch (t->type) {
	case NS_BG_CHECK:
		e = "BG Check";
		cJSON_AddItemToObject(root, "glucose", cJSON_CreateNumber(t->value));
		cJSON_AddItemToObject(root, "units", cJSON_CreateString("mg/dl"));
		break;
	case NS_CORRECTION_BOLUS:
		e = "Correction Bolus";
		cJSON_AddItemToObject(root, "insulin", cJSON_CreateNumber(t->value / 1000.0));
		break;
	case NS_MEAL_BOLUS:
		e = "Meal Bolus";
		cJSON_AddItemToObject(root, "insulin", cJSON_CreateNumber(t->value / 1000.0));
		break;
	case NS_SITE_CHANGE:
		e = "Site Change";
		break;
	case NS_TEMP_BASAL:
		e = "Temp Basal";
		cJSON_AddItemToObject(root, "absolute", cJSON_CreateNumber(t->value / 1000.0));
		cJSON_AddItemToObject(root, "duration", cJSON_CreateNumber(t->minutes));
		break;
	default:
		ESP_LOGE(TAG, "unknown treatment type (%d)", t->type);
		char buf[64];
		sprintf(buf, "unknown (%d)", t->type);
		e = buf;
		break;
	}
	cJSON_AddItemToObject(root, "eventType", cJSON_CreateString(e));
	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}

void upload_treatment(esp_http_client_handle_t client, nightscout_treatment_t *t) {
	char *json = treatment_json(t);
	nightscout_upload(client, "api/v1/treatments", json);
	free(json);
}
