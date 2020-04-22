#include "medtronic_test.h"

history_record_t history[MAX_HISTORY];
int history_length;

static int store_record(history_record_t *r) {
	if (history_length == MAX_HISTORY) {
		return 1;
	}
	history[history_length++] = *r;
	return 0;
}

void parse_data(char *filename, int family) {
	printf("%s (x%d)\n", filename, family);
	FILE *f = fopen(filename, "r");
	if (f == 0) {
		perror(filename);
		exit(1);
	}
	static uint8_t page[HISTORY_PAGE_SIZE];
	int nbytes = read_bytes(f, page, sizeof(page));
	history_length = 0;
	decode_history(page, nbytes, family, store_record);
}

time_t parse_json_time(const char *str) {
	struct tm tm = { .tm_isdst = -1 };
	if (strptime(str, "%FT%T%z", &tm) == 0) {
		fprintf(stderr, "malformed JSON time \"%s\"\n", str);
		exit(1);
	}
	return mktime(&tm);
}

static void malformed(const char *str) {
	fprintf(stderr, "malformed duration string \"%s\"\n", str);
	exit(1);
}

// Parse a duration of the form [[Xh]Ym]Zs.
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
