#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#define TAG "GNARL"

#define SUBG_RFSPY_VERSION "subg_rfspy 1.0"

void gnarl_init(void);
void start_gnarl_task(void);
void rfspy_command(uint8_t *buf, int count);
void send_bytes(uint8_t *buf, int count);
