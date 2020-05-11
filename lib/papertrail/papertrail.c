#include <stdio.h>
#include <netdb.h>

#define TAG		"papertrail"
#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#include <esp_log.h>

#include "papertrail_config.h"

FILE *papertrail;

void papertrail_init(void) {
	struct hostent *he = gethostbyname(PAPERTRAIL_HOST);
        if (!he) {
                ESP_LOGE(TAG, "host %s not found", PAPERTRAIL_HOST);
                return;
        }
        struct in_addr ip_addr = *(struct in_addr *)(he->h_addr_list[0]);
	ESP_LOGI(TAG, "%s has address %s", PAPERTRAIL_HOST, inet_ntoa(ip_addr));

	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (s == -1) {
                ESP_LOGE(TAG, "socket() failed");
		return;
	}

	struct sockaddr_in dest_addr = {
		.sin_family = AF_INET,
		.sin_addr = ip_addr,
		.sin_port = htons(PAPERTRAIL_PORT),
	};
	int err = connect(s, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err == -1) {
                ESP_LOGE(TAG, "connect() failed");
		return;
	}

	papertrail = fdopen(s, "w");
	setvbuf(papertrail, 0, _IONBF, 0);
}
