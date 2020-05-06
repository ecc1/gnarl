#include <stdio.h>
#include <stdlib.h>

#include "medtronic.h"
#include "pump_history.h"

int main(int argc, char **argv) {
	if (argc != 6) {
		fprintf(stderr, "Usage: %s AA BB CC DD EE\n", argv[0]);
		exit(1);
	}
	uint8_t data[5];
	for (int i = 0; i < 5; i++) {
		data[i] = strtol(argv[i + 1], 0, 16);
	}
	char ts[TIME_STRING_SIZE];
	printf("%s\n", time_string(pump_decode_time(data), ts));
}
