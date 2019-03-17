#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "display.h"
#include "gnarl.h"
#include "rfm95.h"

#define MILLISECONDS	1000

#define MAX_PARAM_LEN	10
#define MAX_PACKET_LEN	107

typedef enum {
	cmd_get_state	    = 0x01,
	cmd_get_version	    = 0x02,
	cmd_get_packet	    = 0x03,
	cmd_send_packet	    = 0x04,
	cmd_send_and_listen = 0x05,
	cmd_update_register = 0x06,
	cmd_reset	    = 0x07,
	cmd_led		    = 0x08,
	cmd_read_register   = 0x09,
} rfspy_cmd_t;

typedef struct {
	rfspy_cmd_t command;
	int length;
	uint8_t data[MAX_PARAM_LEN + MAX_PACKET_LEN];
} rfspy_request_t;

#define QUEUE_LENGTH		10

static QueueHandle_t request_queue;

typedef enum {
	ERROR_RX_TIMEOUT      = 0xaa,
	ERROR_CMD_INTERRUPTED = 0xbb,
	ERROR_ZERO_DATA	      = 0xcc,
} rfspy_result_t;

typedef struct {
	uint8_t send_channel;
	uint8_t repeat_count;
	uint8_t delay_ms;
	uint8_t listen_channel;
	uint32_t timeout_ms;
	uint8_t retry_count;
	uint8_t packet[];
} send_and_listen_cmd_t;

typedef struct {
	uint8_t rssi;
	uint8_t packet_count;
	uint8_t packet[MAX_PACKET_LEN];
} response_packet_t;

static response_packet_t rx_buf;

static void send_data(const char *str) {
	send_bytes((uint8_t *)str, strlen(str));
}

static inline void swap_bytes(uint8_t *p, uint8_t *q) {
	uint8_t t = *p;
	*p = *q;
	*q = t;
}

static inline void reverse_four_bytes(uint32_t *n) {
	uint8_t *p = (uint8_t *)n;
	swap_bytes(&p[0], &p[3]);
	swap_bytes(&p[1], &p[2]);
}

static void send_packet(uint8_t *data, int len, int repeat_count, int delay_ms) {
	while (len > 0 && data[len-1] == 0) {
		len--;
	}
	if (len == 0) {
		ESP_LOGE(TAG, "send_packet: len == 0");
		return;
	}
	if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
		printf("sending %d bytes:", len);
		for (int i = 0; i < len; i++) {
			printf(" %02X", data[i]);
		}
		printf("\n");
	}
	transmit(data, len);
	while (repeat_count > 0) {
		usleep(delay_ms  *MILLISECONDS);
		transmit(data, len);
		repeat_count--;
	}
}

// Transform an RSSI value back into the raw encoding that the TI CC111x radios use.
// See section 13.10.3 of the CC1110 data sheet.
static uint8_t raw_rssi(int rssi) {
	const int rssi_offset = 73;
	return (rssi + rssi_offset) * 2;
}

static void send_and_listen(const uint8_t *buf, int len) {
	send_and_listen_cmd_t *p = (send_and_listen_cmd_t *)buf;
	reverse_four_bytes(&p->timeout_ms);
	ESP_LOGD(TAG, "len %d send_channel %d repeat_count %d delay_ms %d",
		 len, p->send_channel, p->repeat_count, p->delay_ms);
	ESP_LOGD(TAG, "listen_channel %d timeout_ms %d retry_count %d",
		 p->listen_channel, p->timeout_ms, p->retry_count);
	len -= (p->packet - (uint8_t *)p);
	send_packet(p->packet, len, p->repeat_count, p->delay_ms);
	int n = receive(rx_buf.packet, sizeof(rx_buf.packet), p->timeout_ms);
	int rssi = read_rssi();
	for (int retries = p->retry_count; retries > 0; retries--) {
		if (n != 0) {
			break;
		}
		send_packet(p->packet, len, p->repeat_count, p->delay_ms);
		n = receive(rx_buf.packet, sizeof(rx_buf.packet), p->timeout_ms);
		rssi = read_rssi();
	}
	if (n == 0) {
		ESP_LOGD(TAG, "send_and_listen: timeout");
		uint8_t err = ERROR_RX_TIMEOUT;
		send_bytes(&err, 1);
		return;
	}
#if DEBUG
	printf("received %d bytes:", n);
	for (int i = 0; i < n; i++) {
		printf(" %02X", rx_buf.packet[i]);
	}
	printf(" (RSSI = %d)\n", rssi);
#endif
	rx_buf.rssi = raw_rssi(rssi);
	if (rx_buf.rssi == 0) {
		rx_buf.rssi = 1;
	}
	rx_buf.packet_count = rx_packet_count();
	if (rx_buf.packet_count == 0) {
		rx_buf.packet_count = 1;
	}
	send_bytes((uint8_t *)&rx_buf, 2 + n);
}

static uint8_t fr[3];

static inline bool valid_frequency(uint32_t f) {
	if (863000000 <= f && f <= 870000000) {
		return true;
	}
	if (910000000 <= f && f <= 920000000) {
		return true;
	}
	return false;
}

// Change the radio frequency if the current register values make sense.
static void check_frequency() {
	uint32_t f = ((uint32_t)fr[0] << 16) + ((uint32_t)fr[1] << 8) + ((uint32_t)fr[2]);
	uint32_t freq = (uint32_t)(((uint64_t)f * 24000000) >> 16);
	if (valid_frequency(freq)) {
		ESP_LOGD(TAG, "setting frequency to %d Hz", freq);
		set_frequency(freq);
	} else {
		ESP_LOGD(TAG, "invalid frequency (%d Hz)", freq);
	}
}

static void update_register(const uint8_t *buf, int len) {
	// AAPS sends 2 bytes, Loop sends 10
	if (len < 2) {
		ESP_LOGE(TAG, "update_register: len = %d", len);
		return;
	}
	uint8_t addr = buf[0];
	uint8_t value = buf[1];
	ESP_LOGD(TAG, "update_register: addr %02X value %02X", addr, value);
	switch (addr) {
	case 0x09 ... 0x0B:
		fr[addr - 0x09] = value;
		check_frequency();
		break;
	default:
		ESP_LOGD(TAG, "update_register: addr %02X ignored", addr);
		break;
	}
	send_data("\x01");
}

void rfspy_command(uint8_t *buf, int count) {
	if (count == 0) {
		ESP_LOGE(TAG, "rfspy_command: count == 0");
		return;
	}
	if (buf[0] != count - 1 || count == 1) {
		ESP_LOGE(TAG, "rfspy_command: length = %d, byte 0 == %d", count, buf[0]);
		return;
	}
	rfspy_cmd_t cmd = buf[1];
	rfspy_request_t req = {
		.command = cmd,
		.length = count - 2,
	};
	memcpy(req.data, buf + 2, req.length);
	if (!xQueueSend(request_queue, &req, 0)) {
		ESP_LOGE(TAG, "rfspy_command: cannot queue request for command %d", cmd);
		return;
	}
	ESP_LOGD(TAG, "rfspy_command: queued command %d, queue length = %d",
		 cmd, uxQueueMessagesWaiting(request_queue));
}

static void gnarl_loop() {
	ESP_LOGD(TAG, "starting gnarl_loop");
	const int timeout_ms = 60000;
	for (;;) {
		rfspy_request_t req;
		if (!xQueueReceive(request_queue, &req, pdMS_TO_TICKS(timeout_ms))) {
			ESP_LOGD(TAG, "gnarl_loop: queue empty");
			continue;
		}
		switch (req.command) {
		case cmd_get_version:
			ESP_LOGD(TAG, "cmd_get_version");
			send_data(SUBG_RFSPY_VERSION);
			break;
		case cmd_send_and_listen:
			ESP_LOGD(TAG, "cmd_send_and_listen");
			send_and_listen(req.data, req.length);
			display_update(PUMP_RSSI, read_rssi());
			break;
		case cmd_update_register:
			ESP_LOGD(TAG, "cmd_update_register");
			update_register(req.data, req.length);
			break;
		default:
			ESP_LOGE(TAG, "unimplemented rfspy command %d", req.command);
			break;
		}
		display_update(COMMAND_TIME, esp_timer_get_time() / 1000000);
	}
}

void start_gnarl_task() {
	request_queue = xQueueCreate(QUEUE_LENGTH, sizeof(rfspy_request_t));
	// Start radio task with high priority to avoid receiving truncated packets.
	xTaskCreate(gnarl_loop, "gnarl", 4096, 0, 24, 0);
}
