#include "4b6b.h"

// Bit-field extraction macros, for uint8_t values only.

#define HI(n, x)  ((x) >> (8 - (n)))
#define LO(n, x)  ((x) & ((1 << (n)) - 1))

static const uint8_t encode_4b[16] = {
	0x15, 0x31, 0x32, 0x23,
	0x34, 0x25, 0x26, 0x16,
	0x1A, 0x19, 0x2A, 0x0B,
	0x2C, 0x0D, 0x0E, 0x1C,
};


int encode_4b6b(const uint8_t *src, uint8_t *dst, size_t len)
{
	int i, n;

	// 2 input bytes produce 3 output bytes.
	for (i = 0, n = 0; i < len - 1; i += 2, n += 3) {
		uint8_t x = src[i], y = src[i + 1];

		uint8_t a = encode_4b[HI(4, x)], b = encode_4b[LO(4, x)];
		uint8_t c = encode_4b[HI(4, y)], d = encode_4b[LO(4, y)];

		dst[n] = (a << 2) | HI(4, b);
		dst[n + 1] = (LO(4, b) << 4) | HI(6, c);
		dst[n + 2] = (LO(2, c) << 6) | d;
	}
	// Odd final input byte, if any, produces 2 output bytes.
	if (i == len - 1) {
		uint8_t x = src[i];

		uint8_t a = encode_4b[HI(4, x)], b = encode_4b[LO(4, x)];

		dst[n++] = (a << 2) | HI(4, b);
		dst[n++] = (LO(4, b) << 4) | 0b0101;	// pad
	}
	return n;
}

// Inverse of encode_4b table, with 0xFF indicating an undefined value.

static const uint8_t decode_6b[64] = {
	/* 0x00 */ 0xFF, /* 0x01 */ 0xFF, /* 0x02 */ 0xFF, /* 0x03 */ 0xFF,
	/* 0x04 */ 0xFF, /* 0x05 */ 0xFF, /* 0x06 */ 0xFF, /* 0x07 */ 0xFF,
	/* 0x08 */ 0xFF, /* 0x09 */ 0xFF, /* 0x0A */ 0xFF, /* 0x0B */ 0x0B,
	/* 0x0C */ 0xFF, /* 0x0D */ 0x0D, /* 0x0E */ 0x0E, /* 0x0F */ 0xFF,
	/* 0x10 */ 0xFF, /* 0x11 */ 0xFF, /* 0x12 */ 0xFF, /* 0x13 */ 0xFF,
	/* 0x14 */ 0xFF, /* 0x15 */ 0x00, /* 0x16 */ 0x07, /* 0x17 */ 0xFF,
	/* 0x18 */ 0xFF, /* 0x19 */ 0x09, /* 0x1A */ 0x08, /* 0x1B */ 0xFF,
	/* 0x1C */ 0x0F, /* 0x1D */ 0xFF, /* 0x1E */ 0xFF, /* 0x1F */ 0xFF,
	/* 0x20 */ 0xFF, /* 0x21 */ 0xFF, /* 0x22 */ 0xFF, /* 0x23 */ 0x03,
	/* 0x24 */ 0xFF, /* 0x25 */ 0x05, /* 0x26 */ 0x06, /* 0x27 */ 0xFF,
	/* 0x28 */ 0xFF, /* 0x29 */ 0xFF, /* 0x2A */ 0x0A, /* 0x2B */ 0xFF,
	/* 0x2C */ 0x0C, /* 0x2D */ 0xFF, /* 0x2E */ 0xFF, /* 0x2F */ 0xFF,
	/* 0x30 */ 0xFF, /* 0x31 */ 0x01, /* 0x32 */ 0x02, /* 0x33 */ 0xFF,
	/* 0x34 */ 0x04, /* 0x35 */ 0xFF, /* 0x36 */ 0xFF, /* 0x37 */ 0xFF,
	/* 0x38 */ 0xFF, /* 0x39 */ 0xFF, /* 0x3A */ 0xFF, /* 0x3B */ 0xFF,
	/* 0x3C */ 0xFF, /* 0x3D */ 0xFF, /* 0x3E */ 0xFF, /* 0x3F */ 0xFF,
};

int decode_4b6b(const uint8_t *src, uint8_t *dst, size_t len)
{
	int i, n;

	// 3 input bytes produce 2 output bytes.
	for (i = 0, n = 0; i < len - 2; i += 3, n += 2) {
		uint8_t x = src[i], y = src[i + 1], z = src[i + 2];

		uint8_t a = decode_6b[HI(6, x)];
		uint8_t b = decode_6b[(LO(2, x) << 4) | HI(4, y)];
		uint8_t c = decode_6b[(LO(4, y) << 2) | HI(2, z)];
		uint8_t d = decode_6b[LO(6, z)];
		if (a == 0xFF || b == 0xFF || c == 0xFF || d == 0xFF)
			return -1;

		dst[n] = (a << 4) | b;
		dst[n + 1] = (c << 4) | d;
	}
	// Final 2 input bytes produce 1 output byte.
	if (i == len - 2) {
		uint8_t x = src[i], y = src[i + 1];

		uint8_t a = decode_6b[HI(6, x)];
		uint8_t b = decode_6b[(LO(2, x) << 4) | HI(4, y)];
		if (a == 0xFF || b == 0xFF)
			return -1;

		dst[n++] = (a << 4) | b;
	} else if (i == len - 1) {
		return -1;	// shouldn't happen
	}
	return n;
}
