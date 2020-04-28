#include "medtronic_test.h"

void usage(void) {
	fprintf(stderr, "Usage: read_history [-m model] data-file\n");
	exit(1);
}

static void print_record(history_record_t *r) {
	int minutes = r->duration / 60;
	char ts[TIME_STRING_SIZE];
	printf("  %s %-18s %5d %4d\n", time_string(r->time, ts), type_string(r->type), r->insulin, minutes);
}

static void print_history(void) {
	printf("%d history records:\n", history_length);
	for (int i = 0; i < history_length; i++) {
		print_record(&history[i]);
	}
}

int main(int argc, char **argv) {
	char *filename;
	char *model = "522";
	if (argc == 4) {
		if (strcmp(argv[1], "-m") != 0) {
			usage();
		}
		model = argv[2];
		filename = argv[3];
	} else if (argc == 2) {
		filename = argv[1];
	} else {
		usage();
	}
	int family = atoi(model) % 100;
	parse_data(filename, family);
	print_history();
}
