#include <stdio.h>

#include "nightscout.h"

static int is_leap_year(int y) {
	if (y % 4 != 0) {
		return 0;
	}
	if (y % 100 != 0) {
		return 1;
	}
	return y % 400 == 0;
}

static int days_per_month(int m, int y) {
	if (m > 6) {
		return 30 + m % 2;
	}
	if (m % 2 == 0) {
		return 31;
	}
	if (m == 1) {
		return 28 + is_leap_year(y);
	}
	return 30;
}

time_t make_gmt(struct tm *tm) {
	int year = 1900 + tm->tm_year;
	time_t days = 0;
	for (int y = 1970; y < year; y++) {
		days += 365 + is_leap_year(y);
	}
	for (int m = 0; m < tm->tm_mon; m++) {
		days += days_per_month(m, year);
	}
	days += tm->tm_mday -1;
	return (days * 24 + tm->tm_hour) * 3600 + tm->tm_min * 60  + tm->tm_sec;
}

char *nightscout_time_string(time_t t) {
	struct tm *tm = localtime(&t);
	static char buf[20];
	strftime(buf, sizeof(buf), "%F %T", tm);
	return buf;
}

struct timeval timeval_from_milliseconds(double ms) {
	struct timeval tv = {
		.tv_sec = ms / 1000,
		.tv_usec = fmod(ms, 1000) * 1000,
	};
	return tv;
}

time_t round_to_seconds(struct timeval tv) {
	time_t t = tv.tv_sec;
	if (tv.tv_usec >= 500000) {
		t++;
	}
	return t;
}

// See https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toISOString

void print_iso_time(char *buf, struct timeval tv) {
	struct tm *tm = gmtime(&tv.tv_sec);
	int n = strftime(buf, ISO_TIME_STRING_SIZE, "%FT%T", tm);
	sprintf(&buf[n], ".%03ldZ", tv.tv_usec / 1000);
}

struct timeval parse_iso_time(const char *str) {
	struct timeval tv = { 0 };
	if (!str) {
		return tv;
	}
	struct tm tm = { 0 };
	int ms;
	int n = sscanf(str, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
		       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		       &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &ms);
	if (n != 7) {
		return tv;
	}
	tm.tm_year -= 1900;
	tm.tm_mon--;
	tv.tv_sec = make_gmt(&tm);
	tv.tv_usec = ms * 1000;
	return tv;
}
