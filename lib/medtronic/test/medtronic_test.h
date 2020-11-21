#include "testing.h"
#include "medtronic.h"
#include "pump_history.h"

#define MAX_HISTORY	150
extern history_record_t history[MAX_HISTORY];
extern int history_length;

void parse_data(char *filename, int family);
void compare_with_json(char *filename);
