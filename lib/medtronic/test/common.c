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
	pump_decode_history(page, nbytes, family, store_record);
	assert(history_length < LEN(history));
}
