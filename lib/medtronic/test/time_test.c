#include "medtronic_test.h"

typedef struct {
	char *byte_str;
	char *time_str;
} decode_time_case_t;

decode_time_case_t decode_time_cases[] = {
	{ "1F 40 00 01 05", "2005-01-01 00:00:31" },
	{ "09 A2 0A 15 10", "2016-02-21 10:34:09" },
	{ "42 22 54 65 10", "2016-04-05 20:34:02" },
	{ "79 23 0C 12 10", "2016-04-18 12:35:57" },
	{ "75 B7 13 04 10", "2016-06-04 19:55:53" },
	{ "5D B3 0F 06 10", "2016-06-06 15:51:29" },
	{ "40 94 12 0F 10", "2016-06-15 18:20:00" },
	{ "B1 34 87 6B 12", "2018-08-11 07:52:49" },
};
#define NUM_DECODE_TIME_CASES	(sizeof(decode_time_cases)/sizeof(decode_time_cases[0]))

void test_decode_time(void) {
	for (int i = 0; i < NUM_DECODE_TIME_CASES; i++) {
		decode_time_case_t *c = &decode_time_cases[i];
		uint8_t *bytes = parse_bytes(c->byte_str);
		time_t t = pump_decode_time(bytes);
		char ts[TIME_STRING_SIZE];
		time_string(t, ts);
		if (strcmp(ts, c->time_str) != 0) {
			test_failed("[%d] pump_decode_time(%s) = %s, want %s", i, c->byte_str, ts, c->time_str);
		}
	}
}

typedef struct {
	char *time_str;
	char *json_str;
} parse_json_time_case_t;

parse_json_time_case_t parse_json_time_cases[] = {
	{ "2005-01-01 00:00:31", "2005-01-01T00:00:31-04:00" },
	{ "2016-02-21 10:34:09", "2016-02-21T10:34:09-04:00" },
	{ "2016-04-05 20:34:02", "2016-04-05T20:34:02-04:00" },
	{ "2016-04-18 12:35:57", "2016-04-18T12:35:57-04:00" },
	{ "2016-06-04 19:55:53", "2016-06-04T19:55:53-04:00" },
	{ "2016-06-06 15:51:29", "2016-06-06T15:51:29-04:00" },
	{ "2016-06-15 18:20:00", "2016-06-15T18:20:00-04:00" },
	{ "2018-08-11 07:52:49", "2018-08-11T07:52:49-04:00" },
};
#define NUM_PARSE_JSON_TIME_CASES	(sizeof(parse_json_time_cases)/sizeof(parse_json_time_cases[0]))

void test_parse_json_time(void) {
	for (int i = 0; i < NUM_PARSE_JSON_TIME_CASES; i++) {
		parse_json_time_case_t *c = &parse_json_time_cases[i];
		time_t t = parse_json_time(c->json_str);
		char ts[TIME_STRING_SIZE];
		time_string(t, ts);
		if (strcmp(ts, c->time_str) != 0) {
			test_failed("[%d] parse_json_time(%s) = %s, want %s", i, c->json_str, ts, c->time_str);
		}
	}
}

typedef struct {
	char *s;
	time_t t;
} parse_duration_case_t;

parse_duration_case_t parse_duration_cases[] = {
	{ "0s", 0 },
	{ "30s", 30 },
	{ "1m0s", 60 },
	{ "1h30m0s", 90 * 60 },
	{ "10h0m0s", 10 * 60 * 60 },
};
#define NUM_PARSE_DURATION_CASES	(sizeof(parse_duration_cases)/sizeof(parse_duration_cases[0]))

void test_parse_duration(void) {
	for (int i = 0; i < NUM_PARSE_DURATION_CASES; i++) {
		parse_duration_case_t *c = &parse_duration_cases[i];
		time_t t = parse_duration(c->s);
		if (t != c->t) {
			test_failed("[%d] parse_duration(%s) = %d, want %d", i, c->s, t, c->t);
		}
	}
}

typedef struct {
	char *ts;
	time_of_day_t s;
} since_midnight_case_t;

since_midnight_case_t since_midnight_cases[] = {
	{ "2015-01-01T09:00:00Z", 9 * 3600 },
	{ "2016-06-15T20:30:00Z", 20 * 3600 + 30 * 60 },
	{ "2010-11-30T23:59:59Z", 23 * 3600 + 59 * 60 + 59 },
};
#define NUM_SINCE_MIDNIGHT_CASES	(sizeof(since_midnight_cases)/sizeof(since_midnight_cases[0]))

void test_since_midnight(void) {
	for (int i = 0; i < NUM_SINCE_MIDNIGHT_CASES; i++) {
		since_midnight_case_t *c = &since_midnight_cases[i];
		time_t t = parse_json_time(c->ts);
		time_t s = since_midnight(t);
		if (s != c->s) {
			test_failed("[%d] since_midnight(%s) = %d, want %d", i, c->ts, s, c->s);
		}
	}
}

int main(int argc, char **argv) {
	test_decode_time();
	test_parse_json_time();
	test_parse_duration();
	test_since_midnight();
	exit_test();
}
