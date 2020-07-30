#ifndef _NIGHTSCOUT_H
#define _NIGHTSCOUT_H

#include <math.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include <esp_http_client.h>

#include "nightscout_config.h"

esp_http_client_handle_t nightscout_client_handle(const char *endpoint);
esp_http_client_handle_t xdrip_client_handle(void);
void nightscout_client_close(esp_http_client_handle_t client);

char *http_get(esp_http_client_handle_t client);

extern time_t http_server_time;

time_t make_gmt(struct tm *tm);

char *nightscout_time_string(time_t t);

struct timeval timeval_from_milliseconds(double ms);
time_t round_to_seconds(struct timeval tv);

#define ISO_TIME_STRING_SIZE	(24 + 1)
void print_iso_time(char *buf, struct timeval tv);
struct timeval parse_iso_time(const char *str);

typedef struct {
	struct timeval tv;
	int sgv;
} nightscout_entry_t;

typedef void (nightscout_entry_callback_t)(const nightscout_entry_t *e);

void process_nightscout_entries(const char *json, nightscout_entry_callback_t callback);

void print_nightscout_entry(const nightscout_entry_t *e);

typedef enum {
	NS_UNKNOWN = 0,
	NS_BG_CHECK,
	NS_CORRECTION_BOLUS,
	NS_MEAL_BOLUS,
	NS_SITE_CHANGE,
	NS_TEMP_BASAL,
} nightscout_treatment_type_t;

typedef struct {
	struct timeval tv;
	nightscout_treatment_type_t type;
	int value;	// glucose in mg/dL or insulin in milliUnits
	int minutes;	// duration of temp basal
} nightscout_treatment_t;

time_t get_last_treatment_time(esp_http_client_handle_t client);

void upload_treatment(esp_http_client_handle_t client, nightscout_treatment_t *t);

typedef struct {
	struct timeval tv;
	int iob;		// milliUnits
	time_t pump_clock;
	int pump_battery;	// milliVolts
	int reservoir;		// milliUnits
	char *pump_status;
	bool bolusing;
	bool suspended;
	// Uploader battery information.
	int battery_percent;	// 0 .. 100
	int battery_voltage;	// milliVolts
} nightscout_device_status_t;

void upload_device_status(esp_http_client_handle_t client, nightscout_device_status_t *s);

void nightscout_upload(esp_http_client_handle_t client, const char *endpoint, const char *json);

#endif // _NIGHTSCOUT_H
