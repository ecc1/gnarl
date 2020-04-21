#include "testing.h"
#include "nightscout.h"

static void test_time(time_t t) {
	struct tm tm;
	gmtime_r(&t, &tm);
	time_t t0 = timegm(&tm);
	if (t0 != t) {
		test_failed("[%s] timegm = %ld", nightscout_time_string(t), t0);
	}
	time_t t1 = make_gmt(&tm);
	if (t1 != t) {
		test_failed("[%s] make_gmt = %ld", nightscout_time_string(t), t1);
	}
}

void test_make_gmt(void) {
	for (time_t t = 1000000000; t <= 3000000000; t += 10000000) {
		test_time(t);
	}
}

int main(int argc, char **argv) {
	test_make_gmt();
	exit_test();
}
