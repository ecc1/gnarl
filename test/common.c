#include "testing.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>

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

time_t parse_json_time(const char *str) {
	struct tm tm = { .tm_isdst = -1 };
	if (strptime(str, "%FT%T%z", &tm) == 0) {
		fprintf(stderr, "malformed JSON time \"%s\"\n", str);
		exit(1);
	}
	return mktime(&tm);
}

time_t parse_time(const char *str) {
	struct tm tm = { .tm_isdst = -1 };
	if (strptime(str, "%FT%H:%M", &tm) == 0) {
		fprintf(stderr, "malformed time \"%s\"\n", str);
		exit(1);
	}
	return mktime(&tm);
}

static void malformed(const char *str) {
	fprintf(stderr, "malformed duration string \"%s\"\n", str);
	exit(1);
}

// Parse a duration of the form [[Xh]Ym]Zs
time_t parse_duration(const char *str) {
	int n = strlen(str);
	if (n < 2 || str[n - 1] != 's') {
		malformed(str);
	}
	time_t t = 0;
	char *s = strchr(str, 'h');
	if (s != 0) {
		int h;
		if (sscanf(str, "%dh", &h) != 1) {
			malformed(str);
		}
		t += 3600 * h;
		str = s + 1;
	}
	s = strchr(str, 'm');
	if (s != 0) {
		int m;
		if (sscanf(str, "%dm", &m) != 1) {
			malformed(str);
		}
		t += 60 * m;
		str = s + 1;
	}
	s = strchr(str, 's');
	if (s != 0) {
		int secs;
		if (sscanf(str, "%ds", &secs) != 1) {
			malformed(str);
		}
		t += secs;
		return t;
	}
	if (s == 0 || s[1] != 0) {
		malformed(str);
	}
	return t;
}
