#include "medtronic_test.h"

typedef struct {
	char *ts;
	int index;
} basal_rate_at_case_t;

basal_rate_at_case_t basal_rate_at_cases[] = {
	{ "2019-06-13T00:00:00Z", 0 },
	{ "2019-06-13T00:59:59Z", 0 },
	{ "2019-06-13T01:00:00Z", 1 },
	{ "2019-06-13T23:00:00Z", 23 },
	{ "2019-06-13T23:59:59Z", 23 },

};
#define NUM_BASAL_RATE_AT_CASES	(sizeof(basal_rate_at_cases)/sizeof(basal_rate_at_cases[0]))

basal_rate_t basal_rate_sched[24];

void test_basal_rate_at(void) {
	for (int h = 0; h < 24; h++) {
		basal_rate_sched[h] = (basal_rate_t){
			.start = h * 3600,
			.rate = 1000,
		};
	}
	for (int i = 0; i < NUM_BASAL_RATE_AT_CASES; i++) {
		basal_rate_at_case_t *c = &basal_rate_at_cases[i];
		time_t at = parse_json_time(c->ts);
		int n = basal_rate_at(basal_rate_sched, 24, at);
		if (n != c->index) {
			test_failed("[%d] basal_rate_at(%s) = %d, want %d", i, c->ts, n, c->index);
		}
	}
}

typedef struct {
	time_t cur;
	time_t next;
} next_change_case_t;

basal_rate_t test_profile[] = {
	{ TOD( 0, 0), 1000 },
	{ TOD( 4, 0), 2000 },
	{ TOD( 8, 0), 3000 },
	{ TOD(12, 0), 4000 },
	{ TOD(16, 0), 5000 },
	{ TOD(20, 0), 6000 },
};

next_change_case_t next_change_cases[] = {
	{ TEST_TIME( 0,  0), TEST_TIME( 4, 0) },
	{ TEST_TIME( 3, 59), TEST_TIME( 4, 0) },
	{ TEST_TIME( 4,  0), TEST_TIME( 8, 0) },
	{ TEST_TIME(23, 59), TEST_TIME(24, 0) },
};
#define NUM_NEXT_CHANGE_CASES	(sizeof(next_change_cases)/sizeof(next_change_cases[0]))

void test_next_change(void) {
	for (int i = 0; i < NUM_NEXT_CHANGE_CASES; i++) {
		next_change_case_t *c = &next_change_cases[i];
		time_t next = next_change(test_profile, LEN(test_profile), c->cur);
		if (next != c->next) {
			char t1[TIME_STRING_SIZE], t2[TIME_STRING_SIZE], t3[TIME_STRING_SIZE];
			test_failed("[%d] next_change(%s) = %s, want %s", i,
				    time_string(c->cur, t1), time_string(next, t2), time_string(c->next, t3));
		}
	}
}

int main(int argc, char **argv) {
	test_basal_rate_at();
	test_next_change();
	exit_test();
}
