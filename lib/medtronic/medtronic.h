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

// Glucose represents a glucose value as either mg/dL or μmol/L,
typedef uint32_t glucose_t;

typedef enum PACKED {
	ABSOLUTE = 0,
	PERCENT	 = 1,
} temp_basal_type_t;

// Insulin is represented in milliUnits.
typedef uint32_t insulin_t;

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
	time_t start; // seconds since midnight
	insulin_t rate;
} basal_rate_t;

typedef struct {
	time_t start; // seconds since midnight
	carb_units_t units;
	int ratio; // 10x grams/unit or 1000x units/exchange
} carb_ratio_t;

typedef struct {
	time_t start; // seconds since midnight
	glucose_units_t units;
	glucose_t sensitivity;
} sensitivity_t;

typedef struct {
	time_t start; // seconds since midnight
	glucose_units_t units;
	glucose_t low;
	glucose_t high;
} target_t;

void pump_set_id(const char *id);

int pump_basal_rates(basal_rate_t *r, int max);
int pump_battery(void);
int pump_carb_ratios(carb_ratio_t *r, int max);
carb_units_t pump_carb_units(void);
time_t pump_clock(void);
int pump_family(void);
glucose_units_t pump_glucose_units(void);
uint8_t *pump_history_page(int page_num);
int pump_model(void);
int pump_reservoir(void);
int pump_sensitivities(sensitivity_t *r, int max);
int pump_settings(settings_t *r);
int pump_status(status_t *r);
int pump_targets(target_t *r, int max);
int pump_temp_basal(int *minutes);
int pump_wakeup(void);

char *time_string(time_t t);
time_t since_midnight(time_t t);

#define DECLARE_SCHEDULE_LOOKUP(type)	type##_t *type##_at(type##_t *r, int len, time_t t)

DECLARE_SCHEDULE_LOOKUP(basal_rate);
DECLARE_SCHEDULE_LOOKUP(carb_ratio);
DECLARE_SCHEDULE_LOOKUP(sensitivity);
DECLARE_SCHEDULE_LOOKUP(target);
