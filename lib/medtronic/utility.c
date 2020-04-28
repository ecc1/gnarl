#include <stdio.h>

#include "medtronic.h"

time_of_day_t since_midnight(time_t t) {
	struct tm *tm = localtime(&t);
	return tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}

char *time_string(time_t t, char *buf) {
	struct tm *tm = localtime(&t);
	strftime(buf, TIME_STRING_SIZE, "%F %T", tm);
	return buf;
}

char *short_time(time_t t, char *buf) {
	struct tm *tm = localtime(&t);
	strftime(buf, SHORT_TIME_SIZE, "%T", tm);
	return buf;
}

char *duration_string(int seconds, char *buf) {
	int s = seconds;
	int h = s / 3600;
	s %= 3600;
	int m = s / 60;
	s %= 60;
	if (h != 0) {
		sprintf(buf, "%dh%dm%ds", h, m, s);
	} else if (m != 0) {
		sprintf(buf, "%dm%ds", m, s);
	} else {
		sprintf(buf, "%ds", s);
	}
	return buf;
}

char *insulin_string(insulin_t ins, char *buf) {
	sprintf(buf, "%d.%03d U", ins / 1000, ins % 1000);
	return buf;
}
