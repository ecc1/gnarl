#ifndef _VOLTAGE_H
#define _VOLTAGE_H

void voltage_init(void);

// Return the battery voltage in millivolts.
// If rawp is non-zero, store the raw ADC reading there.
int voltage_read(int *rawp);

// Convert a battery reading in millivolts to a percentage-full between 0 and 100.
int voltage_level(int mV);

#define VOLTAGE_STRING_SIZE	8
char *voltage_string(int mV, char *buf);

#endif // _VOLTAGE_H
