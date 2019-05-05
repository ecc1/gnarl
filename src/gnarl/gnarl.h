#include <stdint.h>

#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#define MILLISECONDS	1000
#define SECONDS		1000000

#define MHz		1000000

#define TAG "GNARL"

#define BLE_RFSPY_VERSION  "ble_rfspy 0.9"
#define SUBG_RFSPY_VERSION "subg_rfspy 1.0"
#define STATE_OK "OK"

void gnarl_init(void);
void start_gnarl_task(void);
void rfspy_command(const uint8_t *buf, int count);
void send_bytes(const uint8_t *buf, int count);
void print_bytes(const char* msg, const uint8_t *buf, int count);
