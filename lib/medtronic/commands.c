#include <stdio.h>

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
	char buf[20];
	const char *msg;
	switch (n) {
	case 0:
		msg = "empty packet";
		break;
	case NO_RESPONSE:
		// These are too common to log.
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
		sprintf(buf, "error %d", n);
		msg = buf;
		break;
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

static uint8_t *perform(command_t cmd, uint8_t *pkt, int pkt_len, int tries, int rx_timeout, command_t exp_resp, int *result_lenp) {
	if (pkt_len == sizeof(long_buf)) {
		// Don't attempt state-changing commands more than once.
		tries = 1;
	}
	int err = 0;
	for (int t = 0; t < tries; t++) {
		transmit(pkt, pkt_len);
		int n = receive(rx_buf, sizeof(rx_buf), rx_timeout);
		if (n == 0) {
			err = NO_RESPONSE;
			continue;
		}
		n = decode_4b6b(rx_buf, response_buf, n);
		if (n == -1) {
			err = DECODING_FAILURE;
			continue;
		}
		uint8_t c = crc8(response_buf, n-1);
		if (c != response_buf[n-1]) {
			err = CRC_FAILURE;
			continue;
		}
		n--; // discard CRC byte
		if (!valid_response(cmd, exp_resp, n)) {
			err = INVALID_RESPONSE;
			break;
		};
		if (t != 0) {
			ESP_LOGE(TAG, "command %02X required %d tries", cmd, t+1);
		}
		*result_lenp = n - 5;
		return &response_buf[5];
	}
	*result_lenp = err ? err : NO_RESPONSE;
	return 0;
}

#define DEFAULT_TRIES	3
#define DEFAULT_TIMEOUT	500 // milliseconds
#define MAX_NAKS	10

uint8_t *short_command(command_t cmd, int *lenp) {
	encode_short_packet(cmd);
	uint8_t *data = perform(cmd, short_buf, sizeof(short_buf), DEFAULT_TRIES, DEFAULT_TIMEOUT, cmd, lenp);
	int n = *lenp;
	if (n < 0) {
		log_error(cmd, n);
		return 0;
	}
	return data;
}

static uint8_t *acknowledge(command_t cmd, int *lenp) {
	encode_short_packet(CMD_ACK);
	uint8_t *data = perform(CMD_ACK, short_buf, sizeof(short_buf), 1, DEFAULT_TIMEOUT, cmd, lenp);
	int n = *lenp;
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

uint8_t *extended_response(command_t cmd, int *lenp) {
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
			*lenp = seq_num * PAYLOAD_LENGTH;
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
	*lenp = n;
	return 0;
}

uint8_t *check_page_crc(int page_num, int *lenp) {
	uint16_t data_crc = two_byte_be_int(&page_buf[1022]);
	uint16_t calc_crc = crc16(page_buf, 1022);
	if (calc_crc != data_crc) {
		*lenp = -1;
		ESP_LOGE(TAG, "history page %d: computed CRC %04X but received %04X", page_num, calc_crc, data_crc);
		return 0;
	}
	*lenp = HISTORY_PAGE_SIZE;
	return page_buf;
}

uint8_t *handle_no_response(command_t cmd, int page_num, int expected, int *lenp) {
	for (int count = 0; count < MAX_NAKS; count++) {
		encode_short_packet(CMD_NAK);
		int n;
		uint8_t *data = perform(CMD_NAK, short_buf, sizeof(short_buf), 1, DEFAULT_TIMEOUT, cmd, &n);
		if (n < 0) {
			if (n == NO_RESPONSE) {
				continue;
			}
			*lenp = -1;
			log_error(cmd, n);
			return 0;
		}
		uint8_t seq_num = data[0] & ~DONE_BIT;
		ESP_LOGI(TAG, "history page %d: received fragment %d after %d NAK(s)", page_num, seq_num, count + 1);
		return data;
	}
	*lenp = -1;
	ESP_LOGE(TAG, "history page %d: lost fragment %d", page_num, expected);
	return 0;
}

uint8_t *download_page(command_t cmd, int page_num, int *lenp) {
	uint8_t pg = page_num;
	int n;
	uint8_t *data = long_command(cmd, &pg, 1, &n);
	if (n < 0) {
		*lenp = n;
		log_error(cmd, n);
		return 0;
	}
	uint8_t *p = page_buf;
	int expected = 1;
	while (data != 0 && n == FRAGMENT_LENGTH) {
		uint8_t seq_num = data[0] & ~DONE_BIT;
		if (seq_num > expected) {
			// Missed fragment.
			*lenp = -1;
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
				*lenp = -1;
				ESP_LOGE(TAG, "history page %d: missing done bit", page_num);
				return 0;
			}
			return check_page_crc(page_num, lenp);
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
	*lenp = n;
	return 0;
}

bool pump_wakeup(void) {
	int m = pump_get_model();
	if (m != -1) {
		return true;
	}
	encode_short_packet(CMD_WAKEUP);
	int n;
	perform(CMD_WAKEUP, short_buf, sizeof(short_buf), 100, 10, CMD_ACK, &n);
	uint8_t *data = perform(CMD_WAKEUP, short_buf, sizeof(short_buf), 1, 10000, CMD_ACK, &n);
	return data != 0;
}

uint8_t *long_command(command_t cmd, uint8_t *params, int params_len, int *lenp) {
	encode_short_packet(cmd);
	uint8_t *data = perform(cmd, short_buf, sizeof(short_buf), DEFAULT_TRIES, DEFAULT_TIMEOUT, CMD_ACK, lenp);
	int n = *lenp;
	if (n < 0) {
		log_error(cmd, n);
		ESP_LOGE(TAG, "command %02X was not performed", cmd);
		return 0;
	}
	encode_long_packet(cmd, params, params_len);
	data = perform(cmd, long_buf, sizeof(long_buf), 1, DEFAULT_TIMEOUT, CMD_ACK, lenp);
	if (!data) {
		log_error(cmd, n);
		return 0;
	}
	return data;
}
