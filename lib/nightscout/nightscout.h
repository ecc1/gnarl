#ifndef _NIGHTSCOUT_H
#define _NIGHTSCOUT_H

#include <time.h>
#include <sys/time.h>

#include <esp_http_client.h>

esp_http_client_handle_t nightscout_client_handle(void);
esp_http_client_handle_t xdrip_client_handle(void);

char *http_get(esp_http_client_handle_t client);

extern time_t http_server_time;

time_t make_gmt(struct tm *tm);

char *nightscout_time_string(time_t t);

typedef struct {
	struct timeval tv;
	int sgv;
} nightscout_entry_t;

typedef void (nightscout_entry_callback_t)(const nightscout_entry_t *e);

void process_nightscout_entries(const char *json, nightscout_entry_callback_t callback);

void print_nightscout_entry(const nightscout_entry_t *e);

#endif // _NIGHTSCOUT_H
