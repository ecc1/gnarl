#include "testing.h"
#include "medtronic.h"
#include "pump_history.h"

#define MAX_HISTORY	150
history_record_t history[MAX_HISTORY];
int history_length;

void parse_data(char *filename, int family);
const char *type_string(history_record_type_t t);
void compare_with_json(char *filename);
