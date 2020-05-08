#include "testing.h"

typedef struct {
	char *ts;
	insulin_t result;
} basal_rate_at_case_t;

basal_rate_at_case_t basal_rate_at_cases[] = {
	{ "2019-06-13T00:00:00Z", 1000 },
	{ "2019-06-13T00:59:59Z", 1000 },
	{ "2019-06-13T01:00:00Z", 2000 },
	{ "2019-06-13T23:00:00Z", 24000 },
	{ "2019-06-13T23:59:59Z", 24000 },

};
#define NUM_BASAL_RATE_AT_CASES	(sizeof(basal_rate_at_cases)/sizeof(basal_rate_at_cases[0]))

basal_rate_t basal_rate_sched[24];

void test_basal_rate_at(void) {
	for (int h = 0; h < 24; h++) {
		basal_rate_sched[h] = (basal_rate_t){
			.start = h * 3600,
			.rate = (h + 1) * 1000,
		};
	}
	for (int i = 0; i < NUM_BASAL_RATE_AT_CASES; i++) {
		basal_rate_at_case_t *c = &basal_rate_at_cases[i];
		time_t at = parse_json_time(c->ts);
		basal_rate_t *r = basal_rate_at(basal_rate_sched, 24, at);
		if (r->rate != c->result) {
			test_failed("[%d] basal_rate_at(%s) = %d, want %d", i, c->ts, r->rate, c->result);
		}
	}
}

int main(int argc, char **argv) {
	test_basal_rate_at();
	exit_test();
}
