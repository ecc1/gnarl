#include "medtronic.h"

#define DEFINE_SCHEDULE_LOOKUP(type)			\
	DECLARE_SCHEDULE_LOOKUP(type) {			\
		time_t d = since_midnight(t);		\
		int last = -1;				\
		for (int i = 0; i < len; i++, r++) {	\
			if (r->start > d) {		\
				break;			\
			}				\
			last = i;			\
		}					\
		return last;				\
	}

DEFINE_SCHEDULE_LOOKUP(basal_rate)
DEFINE_SCHEDULE_LOOKUP(carb_ratio)
DEFINE_SCHEDULE_LOOKUP(sensitivity)
DEFINE_SCHEDULE_LOOKUP(target)

// next_change returns the time when the next scheduled rate will take effect (strictly after t).
time_t next_change(basal_rate_t *sched, int len, time_t t) {
	int i = basal_rate_at(sched, len, t);
	assert(i >= 0);
	time_of_day_t next = i + 1 < len ? sched[i+1].start : 24 * 3600;
	time_of_day_t delta = next - since_midnight(t);
	return t + delta;
}
