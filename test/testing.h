// This is needed for the declaration of strptime() and timegm()
#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

void test_failed(const char *format, ...);
void exit_test(void);

char *read_file(const char *name);
int read_bytes(FILE *f, uint8_t *buf, int len);
uint8_t *parse_bytes(char *str);

#define LEN(x)	(sizeof(x)/sizeof(x[0]))

int close_enough(double x, double y);

#define TEST_TIME_NOW		1585699200
#define TOD(hh, mm)		((hh) * 3600 + (mm) * 60)
#define TEST_TIME(hh, mm)	(TEST_TIME_NOW + TOD(hh, mm))
