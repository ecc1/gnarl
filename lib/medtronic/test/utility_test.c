#include "medtronic_test.h"

typedef struct {
	insulin_t ins;
	char *str;
} insulin_string_case_t;

insulin_string_case_t insulin_string_cases[] = {
	{      0,   "0.000 U" },
	{   1234,   "1.234 U" },
	{ 123456, "123.456 U" },
	{     -1,  "-0.001 U" },
	{  -1000,  "-1.000 U" },
	{  -1234,  "-1.234 U" },
	{ -98765, "-98.765 U" },
};
#define NUM_INSULIN_STRING_CASES	(sizeof(insulin_string_cases)/sizeof(insulin_string_cases[0]))

void test_insulin_string(void) {
	for (int i = 0; i < NUM_INSULIN_STRING_CASES; i++) {
		insulin_string_case_t *c = &insulin_string_cases[i];
		char str[INSULIN_STRING_SIZE];
		insulin_string(c->ins, str);
		if (strcmp(str, c->str) != 0) {
			test_failed("[%d] insulin_string(%d) = %s, want %s", i, c->ins, str, c->str);
		}
	}
}

int main(int argc, char **argv) {
	test_insulin_string();
	exit_test();
}
