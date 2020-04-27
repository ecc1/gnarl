#ifndef _TESTING_H
#define _TESTING_H

// This is needed for the declaration of strptime() and timegm()
#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define LEN(x)			(sizeof(x)/sizeof(x[0]))

// Some tests assume 00:00 EDT (04:00 GMT).
#define TEST_TIME_NOW		1585713600
#define TOD(hh, mm)		((hh) * 3600 + (mm) * 60)
#define TEST_TIME(hh, mm)	(TEST_TIME_NOW + TOD(hh, mm))

void test_failed(const char *format, ...);
void exit_test(void);

char *read_file(const char *name);
int read_bytes(FILE *f, uint8_t *buf, int len);
uint8_t *parse_bytes(char *str);
time_t parse_time(const char *str);
time_t parse_json_time(const char *str);
time_t parse_duration(const char *str);
int close_enough(double x, double y);

#endif // _TESTING_H
