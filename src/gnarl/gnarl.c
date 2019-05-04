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
	uint8_t listen_channel;
	uint32_t timeout_ms;
} get_packet_cmd_t;

typedef struct {
	uint8_t send_channel;
	uint8_t repeat_count;
	uint8_t delay_ms;
	uint8_t packet[];
} send_packet_cmd_t;

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
	print_bytes("send_packet: sending %d bytes:", data, len);
	transmit(data, len);
	while (repeat_count > 0) {
		usleep(delay_ms  *MILLISECONDS);
		transmit(data, len);
		repeat_count--;
	}
}

static void send_ack() {
	uint8_t ack = 1;
	send_bytes(&ack, sizeof(ack));
}

// Transform an RSSI value back into the raw encoding that the TI CC111x radios use.
// See section 13.10.3 of the CC1110 data sheet.
static uint8_t raw_rssi(int rssi) {
	const int rssi_offset = 73;
	return (rssi + rssi_offset) * 2;
}

static void rx_common(int n, int rssi) {
	if (n == 0) {
		ESP_LOGD(TAG, "RX: timeout");
		uint8_t err = ERROR_RX_TIMEOUT;
		send_bytes(&err, 1);
		return;
	}
	print_bytes("RX: received %d bytes:", rx_buf.packet, n);
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

static void get_packet(const uint8_t *buf, int len) {
	get_packet_cmd_t *p = (get_packet_cmd_t *)buf;
	reverse_four_bytes(&p->timeout_ms);
	ESP_LOGD(TAG, "get_packet: listen_channel %d timeout_ms %d",
		 p->listen_channel, p->timeout_ms);
	int n = receive(rx_buf.packet, sizeof(rx_buf.packet), p->timeout_ms);
	rx_common(n, read_rssi());
}

static void send(const uint8_t *buf, int len) {
	send_packet_cmd_t *p = (send_packet_cmd_t *)buf;
	ESP_LOGD(TAG, "send: len %d send_channel %d repeat_count %d delay_ms %d",
		 len, p->send_channel, p->repeat_count, p->delay_ms);
	len -= (p->packet - (uint8_t *)p);
	send_packet(p->packet, len, p->repeat_count, p->delay_ms);
	send_ack();
}

static void send_and_listen(const uint8_t *buf, int len) {
	send_and_listen_cmd_t *p = (send_and_listen_cmd_t *)buf;
	reverse_four_bytes(&p->timeout_ms);
	ESP_LOGD(TAG, "send_and_listen: len %d send_channel %d repeat_count %d delay_ms %d",
		 len, p->send_channel, p->repeat_count, p->delay_ms);
	ESP_LOGD(TAG, "send_and_listen: listen_channel %d timeout_ms %d retry_count %d",
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
	rx_common(n, rssi);
}

static uint8_t fr[3];

static inline bool valid_frequency(uint32_t f) {
	if (863*MHz <= f && f <= 870*MHz) {
		return true;
	}
	if (910*MHz <= f && f <= 920*MHz) {
		return true;
	}
	return false;
}

// Change the radio frequency if the current register values make sense.
static void check_frequency() {
	uint32_t f = ((uint32_t)fr[0] << 16) + ((uint32_t)fr[1] << 8) + ((uint32_t)fr[2]);
	uint32_t freq = (uint32_t)(((uint64_t)f * 24*MHz) >> 16);
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
	send_ack();
}

static void led_mode(const uint8_t *buf, int len) {
	if (len < 2) {
		ESP_LOGE(TAG, "led_mode: len = %d", len);
		return;
	}
	ESP_LOGD(TAG, "led_mode: led %02X mode %02X", buf[0], buf[1]);
	send_ack();
}

void rfspy_command(const uint8_t *buf, int count) {
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
	ESP_LOGD(TAG, "rfspy_command %d, queue length %d", cmd, uxQueueMessagesWaiting(request_queue));
}

static void gnarl_loop() {
	ESP_LOGD(TAG, "starting gnarl_loop");
	const int timeout_ms = 60*MILLISECONDS;
	for (;;) {
		rfspy_request_t req;
		if (!xQueueReceive(request_queue, &req, pdMS_TO_TICKS(timeout_ms))) {
			continue;
		}
		switch (req.command) {
		case cmd_get_state:
			ESP_LOGD(TAG, "cmd_get_state");
			send_bytes((const uint8_t *)STATE_OK, strlen(STATE_OK));
			break;
		case cmd_get_version:
			ESP_LOGD(TAG, "cmd_get_version");
			send_bytes((const uint8_t *)SUBG_RFSPY_VERSION, strlen(SUBG_RFSPY_VERSION));
			break;
		case cmd_get_packet:
			ESP_LOGD(TAG, "cmd_get_packet");
			get_packet(req.data, req.length);
			break;
		case cmd_send_packet:
			ESP_LOGD(TAG, "cmd_send_packet");
			send(req.data, req.length);
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
		case cmd_led:
			ESP_LOGD(TAG, "cmd_led");
			led_mode(req.data, req.length);
			break;
		default:
			ESP_LOGE(TAG, "unimplemented rfspy command %d", req.command);
			break;
		}
		display_update(COMMAND_TIME, esp_timer_get_time()/SECONDS);
	}
}

void start_gnarl_task() {
	request_queue = xQueueCreate(QUEUE_LENGTH, sizeof(rfspy_request_t));
	// Start radio task with high priority to avoid receiving truncated packets.
	xTaskCreate(gnarl_loop, "gnarl", 4096, 0, 24, 0);
}
