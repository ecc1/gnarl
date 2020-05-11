#ifndef _PAPERTRAIL_H
#define _PAPERTRAIL_H

#include <stdio.h>

extern FILE *papertrail;

void papertrail_init(void);

#undef ESP_LOGE
#define ESP_LOGE(tag, fmt, ...)							\
	do {									\
		fprintf(stderr, fmt "\n", ##__VA_ARGS__);			\
		if (papertrail) fprintf(papertrail, fmt "\n", ##__VA_ARGS__);	\
	} while (0)

#undef ESP_LOGI
#define ESP_LOGI(tag, fmt, ...)							\
	do {									\
		fprintf(stderr, fmt "\n", ##__VA_ARGS__);			\
		if (papertrail) fprintf(papertrail, fmt "\n", ##__VA_ARGS__);	\
	} while (0)

#endif // _PAPERTRAIL_H
