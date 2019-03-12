#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <esp_task_wdt.h>

#include "4b6b.h"
#include "crc.h"
#include "rfm95.h"

#define PUMP_FREQUENCY 868150000 // 916300000
#define TIMEOUT 60 * 1000        // 1 minute, in milliseconds

static uint8_t rx_buf[150], response_buf[100], c;
static uint16_t c16;
static int n;

void app_main() {
  rfm95_init();
  uint8_t v = read_version();
  printf("radio version %d.%d\n", version_major(v), version_minor(v));
  set_frequency(PUMP_FREQUENCY);
  printf("frequency set to %d Hz\n", read_frequency());
  for (;;) {
    esp_task_wdt_reset();

    // receive
    n = sleep_receive(rx_buf, sizeof(rx_buf), TIMEOUT);
    if (n == 0) {
      printf("[timeout]\n");
      continue;
    }
    printf("\n#######################################\n");
    printf("(RSSI = %d, # bytes = %d, count = %d)\nEncoded data:\n",
           read_rssi(), n, rx_packet_count());
    for (int i = 0; i < n; i++) {
      printf("%02X ", rx_buf[i]);
    }
    printf("\n");

    // decode 4b/6b
    n = decode_4b6b(rx_buf, response_buf, n);
    if (n == -1) {
      printf("4b/6b decoding failure\n");
      continue;
    } else {
      printf("Decoded raw data:\n");
      for (int i = 0; i < n; i++) {
        printf("%02X ", response_buf[i]);
      }
      printf("\n");
    }

    // check crc
    c = crc8(response_buf, n - 1);
    if (c != response_buf[n - 1]) {
      printf("CRC8 failure, try CRC16\n");

      c16 = crc16(response_buf, n - 1);
      if (c16 != response_buf[n - 1]) {
        printf("CRC16 failure\n");
        continue;
      } else {
        printf("CRC16 OK\n");
      }
    } else {
      printf("CRC8 OK\n");
    }

    // crop
    // why this ?? copied from pumpstat without understanding it...
    printf("Coped data (5..n-5):\n");
    for (int i = 5; i < n - 5; i++) {
      printf("%02X ", response_buf[i]);
    }
    printf("\n");
  }
}
