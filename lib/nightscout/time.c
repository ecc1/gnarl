#include <time.h>

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
