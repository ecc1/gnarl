#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "medtronic.h"
#include "pump_history.h"

#define MAX_HISTORY	150
history_record_t history[MAX_HISTORY];
int history_length;

const char *type_string(history_record_type_t t);

void parse_data(char *filename, int family);

int read_bytes(FILE *f);
uint8_t *parse_bytes(char *str);
time_t parse_json_time(const char *str);
time_t parse_duration(const char *str);
void compare_with_json(char *filename);

void test_failed(const char *format, ...);
void exit_test(void);

char *read_file(const char *name);
