#define TAG "GNARL"

#define SUBG_RFSPY_VERSION "subg_rfspy 1.0"

void gnarl_init(void);
void start_gnarl_task(void);
void rfspy_command(uint8_t *buf, int count);
void send_bytes(uint8_t *buf, int count);

#define DEBUG 1

#if DEBUG
#undef ESP_LOGD
#define ESP_LOGD(tag, format, ...)	printf(format "\n", ##__VA_ARGS__)

#undef ESP_LOGE
#define ESP_LOGE(tag, format, ...)	printf(format "\n", ##__VA_ARGS__)
#endif
