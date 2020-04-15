#include <sys/time.h>

#include <esp_http_client.h>

esp_http_client_handle_t nightscout_client_handle(void);
esp_http_client_handle_t xdrip_client_handle(const char *hostname);

char *http_get(esp_http_client_handle_t client);

typedef struct {
	struct timeval tv;
	int sgv;
} nightscout_entry_t;

typedef void (nightscout_entry_callback_t)(const nightscout_entry_t *e);

void process_nightscout_entries(const char *json, nightscout_entry_callback_t callback);

void print_nightscout_entry(const nightscout_entry_t *e);
