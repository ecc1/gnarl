#ifndef _MEDTRONIC_H
#define _MEDTRONIC_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define HISTORY_PAGE_SIZE	1022

#define PACKED	__attribute__((packed))

typedef enum PACKED {
	GRAMS	  = 1,
	EXCHANGES = 2,
} carb_units_t;

typedef enum PACKED {
	MG_PER_DL  = 1,
	MMOL_PER_L = 2,
} glucose_units_t;

// Glucose is represented in mg/dL.
typedef int glucose_t;

// The time of day is represented as seconds since midnight.
typedef int time_of_day_t;

typedef enum PACKED {
	ABSOLUTE = 0,
	PERCENT	 = 1,
} temp_basal_type_t;

// Insulin is represented in milliUnits.
typedef int insulin_t;

typedef struct {
	uint8_t dia; // hours
	temp_basal_type_t temp_basal_type;
	insulin_t max_basal;
	insulin_t max_bolus;
} settings_t;

typedef struct {
	uint8_t code;
	uint8_t bolusing;
	uint8_t suspended;
} status_t;
#define STATUS_NORMAL	0x03

typedef struct {
	time_of_day_t start;
	insulin_t rate;
} basal_rate_t;

typedef struct {
	time_of_day_t start;
	carb_units_t units;
	int ratio; // 10x grams/unit or 1000x units/exchange
} carb_ratio_t;

typedef struct {
	time_of_day_t start;
	glucose_units_t units;
	glucose_t sensitivity;
} sensitivity_t;

typedef struct {
	time_of_day_t start;
	glucose_units_t units;
	glucose_t low;
	glucose_t high;
} target_t;

void pump_set_id(const char *id);

int pump_get_basal_rates(basal_rate_t *r, int len);
int pump_get_battery(void);
int pump_get_carb_ratios(carb_ratio_t *r, int len);
carb_units_t pump_get_carb_units(void);
time_t pump_get_clock(void);
int pump_get_family(void);
glucose_units_t pump_get_glucose_units(void);
uint8_t *pump_get_history_page(int page_num);
int pump_get_model(void);
insulin_t pump_get_reservoir(void);
int pump_get_sensitivities(sensitivity_t *r, int len);
int pump_get_settings(settings_t *r);
int pump_get_status(status_t *r);
int pump_get_targets(target_t *r, int len);
insulin_t pump_get_temp_basal(int *minutes);
bool pump_wakeup(void);
int pump_set_temp_basal(int duration_mins, insulin_t rate);

time_of_day_t since_midnight(time_t t);
time_t next_change(basal_rate_t *schedule, int len, time_t t);

#define DECLARE_SCHEDULE_LOOKUP(type)	int type##_at(type##_t *r, int len, time_t t)

DECLARE_SCHEDULE_LOOKUP(basal_rate);
DECLARE_SCHEDULE_LOOKUP(carb_ratio);
DECLARE_SCHEDULE_LOOKUP(sensitivity);
DECLARE_SCHEDULE_LOOKUP(target);

// YYYY-MM-DD HH:MM:SS
#define TIME_STRING_SIZE	20
char *time_string(time_t t, char *buf);

char *format_time(time_t t, const char *fmt, char *buf, int len);

#define DURATION_STRING_SIZE	20
char *duration_string(int seconds, char *buf);

#define INSULIN_STRING_SIZE	16
char *insulin_string(insulin_t ins, char *buf);

#endif // _MEDTRONIC_H
