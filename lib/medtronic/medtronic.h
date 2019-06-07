#include <time.h>

typedef enum {
	GRAMS	  = 1,
	EXCHANGES = 2,
} carb_units_t;

typedef enum {
	MG_PER_DL  = 1,
	MMOL_PER_L = 2,
} glucose_units_t;

typedef enum {
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
	uint8_t start; // half-hours
	insulin_t rate;
} basal_rate_t;

void pump_set_id(const char *id);

int pump_basal_rates(basal_rate_t *r, int max);
int pump_battery(void);
carb_units_t pump_carb_units(void);
int pump_clock(struct tm *tm);
int pump_family(void);
glucose_units_t pump_glucose_units(void);
int pump_model(void);
int pump_reservoir(void);
int pump_settings(settings_t *r);
int pump_status(status_t *r);
int pump_temp_basal(int *minutes);
int pump_wakeup(void);
