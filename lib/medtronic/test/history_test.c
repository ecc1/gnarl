#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "medtronic.h"
#include "pump_history.h"

int family = 22;

#define MAX_HISTORY	150
history_record_t history[MAX_HISTORY];

uint8_t page[HISTORY_PAGE_SIZE + 2];

int read_bytes(const char *filename) {
	FILE *f = fopen(filename, "r");
	if (f == 0) {
		perror(filename);
		exit(1);
	}
	int n = 0;
	for (;;) {
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
		char buf[3];
		buf[0] = c;
		buf[1] = fgetc(f);
		buf[2] = 0;
		uint8_t b = strtol(buf, 0, 16);
		page[n] = b;
		n++;
	}
	fclose(f);
	return n;
}

void print_record(history_record_t *r) {
	int minutes = r->duration / 60;
	printf("  %s %02X %5d %4d\n", time_string(r->time), r->type, r->insulin, minutes);
}

int main(int argc, char **argv) {
	putenv("TZ=UTC");
	if (argc != 2) {
		fprintf(stderr, "Usage: %s raw-data-file\n", argv[0]);
		exit(1);
	}
	int nbytes = read_bytes(argv[1]);
	int n = decode_history(page, family, history, nbytes);
	printf("%d history records:\n", n);
	for (int i = 0; i < n; i++) {
		print_record(&history[i]);
	}
}
