#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "4b6b.h"
#include "commands.h"
#include "crc.h"
#include "rfm95.h"

#define CARELINK_DEVICE	0xA7

#define CMD_ACK		0x06
#define CMD_NAK		0x15
#define CMD_WAKEUP	0x5D
#define CMD_BATTERY	0x72
#define CMD_RESERVOIR	0x73
#define CMD_MODEL	0x8D
#define CMD_TEMP_BASAL	0x98

#define NO_RESPONSE		-1
#define DECODING_FAILURE	-2
#define CRC_FAILURE		-3
#define INVALID_RESPONSE	-4

static const char *err_messages[] = {
	"???",
	"no response",
	"decoding failure",
	"CRC failure",
	"invalid response",
};

static uint8_t pump_id[3];

void parse_pump_id(const char *id) {
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

//static long_packet_t long_Packet;

// 71-byte long packet encodes to 107 bytes.
static uint8_t long_buf[107];

static void encode_pump_id(uint8_t *dst) {
	dst[0] = pump_id[0];
	dst[1] = pump_id[1];
	dst[2] = pump_id[2];
}

static void encode_short_packet(uint8_t cmd) {
	short_packet.device_type = CARELINK_DEVICE;
	encode_pump_id(short_packet.pump_id);
	short_packet.command = cmd;
	short_packet.length = 0;
	uint8_t *p = (uint8_t *)&short_packet;
	short_packet.crc = crc8(p, sizeof(short_packet)-1);
	encode_4b6b(p, short_buf, sizeof(short_packet));
}

static uint8_t rx_buf[150], response_buf[100];

static int valid_response(uint8_t cmd, uint8_t resp, int n) {
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
	if (p->command == cmd || p->command == resp) {
		return 1;
	}
	return 0;
}

static uint8_t *perform(uint8_t cmd, uint8_t *pkt, int pkt_len, int tries, int rx_timeout, uint8_t exp_resp, int *result_len) {
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
	if (!valid_response(cmd, exp_resp, n)) {
		*result_len = INVALID_RESPONSE;
		return 0;
	}
	*result_len = n - 5;
	return &response_buf[5];
}

#define DEFAULT_TRIES 3
#define DEFAULT_TIMEOUT 500 // milliseconds

static uint8_t *short_command(uint8_t cmd, int *len) {
	encode_short_packet(cmd);
	uint8_t *data = perform(cmd, short_buf, sizeof(short_buf), DEFAULT_TRIES, DEFAULT_TIMEOUT, cmd, len);
	int n = *len;
	if (n < 0) {
		if (n != NO_RESPONSE) {
			printf("%s\n", err_messages[-n]);
		}
		return 0;
	}
	return data;
}

static int cached_pump_family;

int pump_model() {
	int n;
	uint8_t *data = short_command(CMD_MODEL, &n);
	if (!data || n < 2) {
		return -1;
	}
	int k = data[1];
	if (n < 2 + k) {
		return -1;
	}
	int model = 0;
	for (int i = 2; i < 2 + k; i++) {
		model = 10*model + data[i] - '0';
	}
	cached_pump_family = model % 100;
	return model;
}

static inline int two_byte_int(uint8_t *p) {
	return (p[0] << 8) | p[1];
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

int pump_family() {
	if (cached_pump_family == 0) {
		pump_model();
	}
	return cached_pump_family;
}

int pump_battery() {
	int n;
	uint8_t *data = short_command(CMD_BATTERY, &n);
	if (!data || n < 4 || data[0] != 3) {
		return -1;
	}
	return two_byte_int(&data[2]) * 10;
}

int pump_reservoir() {
	int n;
	uint8_t *data = short_command(CMD_RESERVOIR, &n);
	if (!data) {
		return -1;
	}
	int fam = pump_family();
	if (fam <= 22) {
		if (n < 3 || data[0] != 2) {
			return -1;
		}
		return two_byte_int(&data[1]) * 100;
	}
	if (n < 5 || data[0] != 4) {
		return -1;
	}
	return two_byte_int(&data[3]) * 25;
}

int pump_temp_basal(int *minutes) {
	int n;
	uint8_t *data = short_command(CMD_TEMP_BASAL, &n);
	if (!data || n < 7 || data[0] != 6) {
		return -1;
	}
	*minutes = two_byte_int(&data[5]);
	if (data[1] != 0) {
		printf("unsupported %d percent temp basal\n", data[2]);
		return 0;
	}
	return two_byte_int(&data[3]) * 25;
}
