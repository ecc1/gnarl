#include <stdint.h>

#include "medtronic.h"

time_t since_midnight(time_t t) {
	struct tm *tm = localtime(&t);
	return tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}

#define DEFINE_SCHEDULE_LOOKUP(type)			\
	DECLARE_SCHEDULE_LOOKUP(type) {			\
		time_t d = since_midnight(t);		\
		type##_t *last = 0;			\
		for (int i = 0; i < len; i++, r++) {	\
			if (r->start > d) {		\
				break;			\
			}				\
			last = r;			\
		}					\
		return last;				\
	}

DEFINE_SCHEDULE_LOOKUP(basal_rate)
DEFINE_SCHEDULE_LOOKUP(carb_ratio)
DEFINE_SCHEDULE_LOOKUP(sensitivity)
DEFINE_SCHEDULE_LOOKUP(target)
