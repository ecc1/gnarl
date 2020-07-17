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

typedef struct {
	double ms;
	struct timeval tv;
} timeval_from_milliseconds_case_t;

timeval_from_milliseconds_case_t timeval_from_milliseconds_cases[] = {
	{ 0, { 0, 0 }},
	{ 1600000000000, { 1600000000, 0 } },
	{ 1600000000500, { 1600000000, 500000 } },
	{ 1600000000999, { 1600000000, 999000 } },
	{ 1586832422769, { 1586832422, 769000 } },
	{ 1586831822624, { 1586831822, 624000 } },
};
#define NUM_TIMEVAL_FROM_MILLISECONDS_CASES	(sizeof(timeval_from_milliseconds_cases)/sizeof(timeval_from_milliseconds_cases[0]))

struct timeval timeval_from_milliseconds(double ms);

void test_timeval_from_milliseconds(void) {
	for (int i = 0; i < NUM_TIMEVAL_FROM_MILLISECONDS_CASES; i++) {
		timeval_from_milliseconds_case_t *c = &timeval_from_milliseconds_cases[i];
		struct timeval tv = timeval_from_milliseconds(c->ms);
		if (tv.tv_sec != c->tv.tv_sec || tv.tv_usec != c->tv.tv_usec) {
			test_failed("[%d] timeval_from_milliseconds(%.0f) = { %ld, %ld }, want { %ld, %ld }",
				    i, c->ms, tv.tv_sec, tv.tv_usec, c->tv.tv_sec, c->tv.tv_usec);
		}
		time_t t1 = round_to_seconds(tv);
		time_t t2 = (time_t)round(c->ms / 1000.0);
		if (t1 != t2) {
			test_failed("[%d] round_to_seconds(%.0f) = %ld, want %ld", i, c->ms, t1, t2);
		}
	}
}

typedef struct {
	char *iso_str;
	char *time_str;
} iso_time_case_t;

iso_time_case_t iso_time_cases[] = {
	{ "2005-01-01T00:00:31.000Z", 0 },
	{ "2016-06-15T18:20:00.000Z", 0 },
	{ "2019-12-31T23:59:59.001Z", "2019-12-31T23:59:59.000Z" },
	{ "2019-12-31T23:59:59.500Z", "2020-01-01T00:00:00.000Z" },
	{ "2019-12-31T23:59:59.999Z", "2020-01-01T00:00:00.000Z" },
};
#define NUM_ISO_TIME_CASES	(sizeof(iso_time_cases)/sizeof(iso_time_cases[0]))

void test_iso_time(void) {
	char ts[ISO_TIME_STRING_SIZE];
	for (int i = 0; i < NUM_ISO_TIME_CASES; i++) {
		iso_time_case_t *c = &iso_time_cases[i];
		struct timeval tv = parse_iso_time(c->iso_str);
		print_iso_time(ts, tv);
		if (strcmp(ts, c->iso_str) != 0) {
			test_failed("[%d] print_iso_time(parse_iso_time(%s)) = %s", i, c->iso_str, ts);
		}
		time_t t = round_to_seconds(tv);
		struct timeval tv2 = c->time_str ? parse_iso_time(c->time_str) : tv;
		if (t != tv2.tv_sec) {
			test_failed("[%d] round_to_seconds(%s) = %ld, want %ld", i, c->iso_str, t, tv2.tv_sec);
		}
	}
}

int main(int argc, char **argv) {
	test_make_gmt();
	test_timeval_from_milliseconds();
	test_iso_time();
	exit_test();
}
