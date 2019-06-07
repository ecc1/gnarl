#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "medtronic.h"
#include "4b6b.h"
#include "commands.h"
#include "crc.h"
#include "rfm95.h"

#define CARELINK_DEVICE		0xA7

#define PAYLOAD_LENGTH		64
#define FRAGMENT_LENGTH		(PAYLOAD_LENGTH + 1) // seq# + payload
#define DONE_BIT		(1 << 7)

#define NO_RESPONSE		-1
#define DECODING_FAILURE	-2
#define CRC_FAILURE		-3
#define INVALID_RESPONSE	-4

static void log_error(command_t cmd, int n) {
	const char *msg;
	switch (n) {
	case NO_RESPONSE:
		return;
	case DECODING_FAILURE:
		msg = "decoding failure";
		break;
	case CRC_FAILURE:
		msg = "CRC failure";
		break;
	case INVALID_RESPONSE:
		msg = "invalid response";
		break;
	default:
		ESP_LOGE(TAG, "command %02X: error %d", cmd, n);
		return;
	}
	ESP_LOGE(TAG, "command %02X: %s", cmd, msg);
}

void print_bytes(const char *msg, const uint8_t *buf, int count) {
	if (LOG_LOCAL_LEVEL < ESP_LOG_DEBUG) {
		return;
	}
	printf(msg, count);
	for (int i = 0; i < count; i++) {
		printf(" %02X", buf[i]);
	}
	printf("\n");
}

static uint8_t pump_id[3];

void pump_set_id(const char *id) {
	uint32_t n = 0;
	int i = 0;
	for (;;) {
		char c = id[i];
		if (c == 0) {
			break;
		}
		n <<= 4;
		if ('0' <= c && c <= '9') {
			n += c - '0';
		} else if ('A' <= c && c <= 'F') {
			n += 10 + c - 'A';
		} else if ('a' <= c && c <= 'f') {
			n += 10 + c - 'a';
		}
		i++;
	}
	pump_id[0] = n >> 16;
	pump_id[1] = n >> 8;
	pump_id[2] = n;
}

typedef struct {
	uint8_t device_type;
	uint8_t pump_id[3];
	uint8_t command;
	uint8_t length;
	uint8_t crc;
} short_packet_t;

static short_packet_t short_packet;

// 7-byte short packet encodes to 11 bytes.
static uint8_t short_buf[11];

typedef struct {
	uint8_t device_type;
	uint8_t pump_id[3];
	uint8_t command;
	uint8_t length;
	uint8_t params[64];
	uint8_t crc;
} long_packet_t;

// 71-byte long packet encodes to 107 bytes.
static uint8_t long_buf[107];

static void encode_pump_id(uint8_t *dst) {
	dst[0] = pump_id[0];
	dst[1] = pump_id[1];
	dst[2] = pump_id[2];
}

static void encode_short_packet(command_t cmd) {
	short_packet.device_type = CARELINK_DEVICE;
	encode_pump_id(short_packet.pump_id);
	short_packet.command = cmd;
	short_packet.length = 0;
	uint8_t *p = (uint8_t *)&short_packet;
	short_packet.crc = crc8(p, sizeof(short_packet)-1);
	encode_4b6b(p, short_buf, sizeof(short_packet));
}

static uint8_t rx_buf[150], response_buf[100];

static int valid_response(command_t cmd, command_t resp, int n) {
	if (n < 6) {
		return 0;
	}
	short_packet_t *p = (short_packet_t *)response_buf;
	if (p->device_type != CARELINK_DEVICE) {
		return 0;
	}
	if (memcmp(p->pump_id, pump_id, sizeof(pump_id)) != 0) {
		return 0;
	}
	return p->command == cmd || p->command == resp;
}

static uint8_t *perform(command_t cmd, uint8_t *pkt, int pkt_len, int tries, int rx_timeout, command_t exp_resp, int *result_len) {
	if (pkt_len == sizeof(long_buf)) {
		// Don't attempt state-changing commands more than once.
		tries = 1;
	}
	int n = 0;
	for (int t = 0; t < tries; t++) {
		transmit(pkt, pkt_len);
		n = receive(rx_buf, sizeof(rx_buf), rx_timeout);
		if (n != 0) {
			break;
		}
	}
	if (n == 0) {
		*result_len = NO_RESPONSE;
		return 0;
	}
	n = decode_4b6b(rx_buf, response_buf, n);
	if (n == -1) {
		*result_len = DECODING_FAILURE;
		return 0;
	}
	uint8_t c = crc8(response_buf, n-1);
	if (c != response_buf[n-1]) {
		*result_len = CRC_FAILURE;
		return 0;
	}
	n--; // discard CRC byte
	if (!valid_response(cmd, exp_resp, n)) {
		*result_len = INVALID_RESPONSE;
		return 0;
	}
	*result_len = n - 5;
	return &response_buf[5];
}

#define DEFAULT_TRIES 3
#define DEFAULT_TIMEOUT 500 // milliseconds

uint8_t *short_command(command_t cmd, int *len) {
	encode_short_packet(cmd);
	uint8_t *data = perform(cmd, short_buf, sizeof(short_buf), DEFAULT_TRIES, DEFAULT_TIMEOUT, cmd, len);
	int n = *len;
	if (n < 0) {
		log_error(cmd, n);
		return 0;
	}
	return data;
}

static uint8_t *acknowledge(command_t cmd, int *len) {
	encode_short_packet(CMD_ACK);
	uint8_t *data = perform(CMD_ACK, short_buf, sizeof(short_buf), 1, DEFAULT_TIMEOUT, cmd, len);
	int n = *len;
	if (n < 0) {
		log_error(cmd, n);
		return 0;
	}
	return data;
}

static uint8_t page_buf[1024];

uint8_t *extended_response(command_t cmd, int *len) {
	int n;
	int expected = 1;
	uint8_t *p = page_buf;
	uint8_t *data = short_command(cmd, &n);
	while (n == FRAGMENT_LENGTH) {
		uint8_t seq_num = data[0] & ~DONE_BIT;
		if (seq_num != expected) {
			ESP_LOGD(TAG, "command %02X: received fragment %d instead of %d", cmd, seq_num, expected);
			break;
		}
		memcpy(p, data + 1, PAYLOAD_LENGTH);
		p += PAYLOAD_LENGTH;
		if (data[0] & DONE_BIT) {
			*len = seq_num * PAYLOAD_LENGTH;
			return page_buf;
		}
		// Acknowledge this fragment and receive the next.
		data = acknowledge(cmd, &n);
		expected++;
	}
	if (n < 0) {
		log_error(cmd, n);
	} else {
		ESP_LOGD(TAG, "command %02X: received %d-byte response", cmd, n);
		print_bytes("response", data, n);
	}
	*len = -1;
	return 0;
}

int pump_wakeup() {
	int m = pump_model();
	if (m != -1) {
		return 1;
	}
	encode_short_packet(CMD_WAKEUP);
	int n;
	perform(CMD_WAKEUP, short_buf, sizeof(short_buf), 100, 10, CMD_ACK, &n);
	uint8_t *data = perform(CMD_WAKEUP, short_buf, sizeof(short_buf), 1, 10000, CMD_ACK, &n);
	return data != 0;
}
