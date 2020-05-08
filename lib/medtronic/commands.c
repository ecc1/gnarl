#include <stdint.h>
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
#define NUM_FRAGMENTS		16

#define NO_RESPONSE		-1
#define DECODING_FAILURE	-2
#define CRC_FAILURE		-3
#define INVALID_RESPONSE	-4

static void log_error(command_t cmd, int n) {
	const char *msg;
	switch (n) {
	case 0:
		msg = "empty packet";
		break;
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

static void encode_pump_id(uint8_t *dst) {
	dst[0] = pump_id[0];
	dst[1] = pump_id[1];
	dst[2] = pump_id[2];
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

static void encode_short_packet(command_t cmd) {
	short_packet.device_type = CARELINK_DEVICE;
	encode_pump_id(short_packet.pump_id);
	short_packet.command = cmd;
	short_packet.length = 0;
	uint8_t *p = (uint8_t *)&short_packet;
	short_packet.crc = crc8(p, sizeof(short_packet)-1);
	encode_4b6b(p, short_buf, sizeof(short_packet));
}

typedef struct {
	uint8_t device_type;
	uint8_t pump_id[3];
	uint8_t command;
	uint8_t length;
	uint8_t params[64];
	uint8_t crc;
} long_packet_t;

static long_packet_t long_packet;

// 71-byte long packet encodes to 107 bytes.
static uint8_t long_buf[107];

static void encode_long_packet(command_t cmd, uint8_t *params, int len) {
	long_packet.device_type = CARELINK_DEVICE;
	encode_pump_id(long_packet.pump_id);
	long_packet.command = cmd;
	long_packet.length = len;
	memcpy(long_packet.params, params, len);
	memset(&long_packet.params[len], 0, sizeof(long_packet.params) - len);
	uint8_t *p = (uint8_t *)&long_packet;
	long_packet.crc = crc8(p, sizeof(long_packet)-1);
	encode_4b6b(p, long_buf, sizeof(long_packet));
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

#define DEFAULT_TRIES	3
#define DEFAULT_TIMEOUT	500 // milliseconds
#define MAX_NAKS	10

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

typedef struct {
	uint8_t data[HISTORY_PAGE_SIZE];
	uint8_t crc[2];
} history_page_t;

static uint8_t page_buf[1024];

uint8_t *extended_response(command_t cmd, int *len) {
	int n;
	int expected = 1;
	uint8_t *p = page_buf;
	uint8_t *data = short_command(cmd, &n);
	while (data != 0 && n == FRAGMENT_LENGTH) {
		uint8_t seq_num = data[0] & ~DONE_BIT;
		if (seq_num != expected) {
			ESP_LOGE(TAG, "command %02X: received fragment %d instead of %d", cmd, seq_num, expected);
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
		ESP_LOGE(TAG, "command %02X: received %d-byte response", cmd, n);
		print_bytes("response", data, n);
		n = -1;
	}
	*len = n;
	return 0;
}

uint8_t *check_page_crc(int page_num, int *len) {
	uint16_t data_crc = two_byte_be_int(&page_buf[1022]);
	uint16_t calc_crc = crc16(page_buf, 1022);
	if (calc_crc != data_crc) {
		*len = -1;
		ESP_LOGE(TAG, "history page %d: computed CRC %04X but received %04X", page_num, calc_crc, data_crc);
		return 0;
	}
	*len = HISTORY_PAGE_SIZE;
	return page_buf;
}

uint8_t *handle_no_response(command_t cmd, int page_num, int expected, int *len) {
	for (int count = 0; count < MAX_NAKS; count++) {
		encode_short_packet(CMD_NAK);
		int n;
		uint8_t *data = perform(CMD_NAK, short_buf, sizeof(short_buf), 1, DEFAULT_TIMEOUT, cmd, &n);
		if (n <= 0) {
			if (n == NO_RESPONSE) {
				continue;
			}
			*len = -1;
			log_error(cmd, n);
			return 0;
		}
		uint8_t seq_num = data[0] & ~DONE_BIT;
		ESP_LOGI(TAG, "history page %d: received fragment %d after %d NAK(s)", page_num, seq_num, count + 1);
		return data;
	}
	*len = -1;
	ESP_LOGE(TAG, "history page %d: lost fragment %d", page_num, expected);
	return 0;
}

uint8_t *download_page(command_t cmd, int page_num, int *len) {
	encode_short_packet(cmd);
	int n;
	uint8_t *data = perform(cmd, short_buf, sizeof(short_buf), DEFAULT_TRIES, DEFAULT_TIMEOUT, CMD_ACK, &n);
	if (n < 0) {
		*len = n;
		log_error(cmd, n);
		return 0;
	}
	int expected = 1;
	uint8_t *p = page_buf;
	uint8_t pg = page_num;
	encode_long_packet(cmd, &pg, sizeof(pg));
	data = perform(cmd, long_buf, sizeof(long_buf), DEFAULT_TRIES, 2*DEFAULT_TIMEOUT, CMD_ACK, &n);
	while (data != 0 && n == FRAGMENT_LENGTH) {
		uint8_t seq_num = data[0] & ~DONE_BIT;
		if (seq_num > expected) {
			// Missed fragment.
			*len = -1;
			ESP_LOGE(TAG, "history page %d: received fragment %d instead of %d", page_num, seq_num, expected);
			return 0;
		}
		if (seq_num == expected) {
			memcpy(p, data + 1, PAYLOAD_LENGTH);
			p += PAYLOAD_LENGTH;
			expected++;
		}
		if (seq_num == NUM_FRAGMENTS) {
			if (!(data[0] & DONE_BIT)) {
				*len = -1;
				ESP_LOGE(TAG, "history page %d: missing done bit", page_num);
				return 0;
			}
			return check_page_crc(page_num, len);
		}
		// Acknowledge this fragment and receive the next.
		data = acknowledge(cmd, &n);
		if (n < 0) {
			if (n != NO_RESPONSE) {
				break;
			}
			data = handle_no_response(cmd, page_num, expected, &n);
		}

	}
	if (n < 0) {
		log_error(cmd, n);
	} else {
		ESP_LOGE(TAG, "history page %d: received %d-byte response", page_num, n);
		print_bytes("response", data, n);
		n = -1;
	}
	*len = n;
	return 0;
}

int pump_wakeup(void) {
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
