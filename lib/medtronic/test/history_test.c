#include "medtronic_test.h"

static char *test_data_dir = "testdata";

static char *test_files[] = {
	"model-512-1",
	"model-512-2",
	"model-515",
	"model-522",
	"model-523-1",
	"model-523-2",
	"ps2-522-1",
	"ps2-522-2",
	"ps2-523-1",
	"ps2-523-2",
	"ps2-523-3",
	"ps2-523-4",
	"ps2-523-5",
	"ps2-523-6",
	"ps2-551-1",
	"ps2-551-2",
	"ps2-551-3",
	"ps2-551-4",
	"ps2-554-1",
	"ps2-554-2",
	"ps2-554-3",
	"ps2-554-4",
	"ps2-554-5",
};
#define NUM_TEST_FILES	(sizeof(test_files)/sizeof(test_files[0]))

static char *get_data_file(char *name, int *familyp) {
	static char buf[50];
	strcpy(buf, strchr(name, '-') + 1);
	char *t = strchr(buf, '-');
	if (t != 0) {
		*t = 0;
	}
	*familyp = atoi(buf) % 100;
	sprintf(buf, "%s/%s.data", test_data_dir, name);
	return buf;
}

static char *get_json_file(char *name) {
	static char buf[50];
	sprintf(buf, "%s/%s.json", test_data_dir, name);
	return buf;
}

static void run_test(char *name) {
	int family;
	char *data_file = get_data_file(name, &family);
	char *json_file = get_json_file(name);
	parse_data(data_file, family);
	compare_with_json(json_file);
}

int main(int argc, char **argv) {
	for (int i = 0; i < NUM_TEST_FILES; i++) {
		run_test(test_files[i]);
	}
	exit_test();
}
