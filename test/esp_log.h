#ifndef _ESP_LOG_H
#define _ESP_LOG_H

// Dummy header file for compiling test programs.

#include <stdbool.h>
#include <stdio.h>

#define ESP_LOGE(tag, fmt, ...)	fprintf(stderr, fmt "\n", ##__VA_ARGS__)

// Avoid compiler complaints about unused variables by leaving them in (unreachable) code.
#define ESP_LOGI(tag, fmt, ...)	if (true) {} else fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#endif // _ESP_LOG_H
