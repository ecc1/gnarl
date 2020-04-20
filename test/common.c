#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#include "testing.h"

static const double tolerance = 1e-06;

int close_enough(double x, double y) {
	return fabs(x - y) <= tolerance;
}

static int test_failures = 0;

void test_failed(const char *format, ...) {
	test_failures++;
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "FAIL ");
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

void exit_test(void) {
	exit(test_failures);
}

#define MAX_FILE_SIZE	32768

char *read_file(const char *name) {
	static char buf[MAX_FILE_SIZE];
	FILE *f = fopen(name, "r");
	assert(f != NULL);
	int i = 0;
	for (;;) {
		assert(i < sizeof(buf)-1);
		int n = fread(&buf[i], 1, sizeof(buf)-1-i, f);
		if (n == 0) {
			break;
		}
		i += n;
	}
	buf[i] = '\0';
	return buf;
}

int read_bytes(FILE *f, uint8_t *buf, int len) {
	int n;
	for (n = 0; n < len; n++) {
		int c;
		for (;;) {
			c = fgetc(f);
			if (c == EOF) {
				fclose(f);
				return n;
			}
			if (!isspace(c)) {
				break;
			}
		}
		char s[3];
		s[0] = c;
		s[1] = fgetc(f);
		s[2] = 0;
		uint8_t b = strtol(s, 0, 16);
		buf[n] = b;
	}
	fclose(f);
	return n;
}

uint8_t *parse_bytes(char *str) {
	static uint8_t buf[256];
	read_bytes(fmemopen(str, strlen(str), "r"), buf, sizeof(buf));
	return buf;
}
