#include <time.h>

void parse_pump_id(const char *id);

int pump_battery(void);
int pump_clock(struct tm *tm);
int pump_family(void);
int pump_model(void);
int pump_reservoir(void);
int pump_temp_basal(int *minutes);
int pump_wakeup(void);
