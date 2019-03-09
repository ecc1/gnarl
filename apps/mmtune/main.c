#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#define ARDUINO_RUNNING_CORE 1

#include "commands.h"
#include "oled.h"
#include "pump.h"
#include "rfm95.h"

#define TAG "mmtune"

#define SECONDS 1000000
#define DISPLAY_TIMEOUT (30 * SECONDS)

#define FP_FQ(n) (int)(n) / 1000000, (int)((n) % 1000000) / 1000

int model;
int rssi;
int try_frequency(long frequency) {
  set_frequency(frequency);
  printf("frequency set to %d Hz\n", read_frequency());
  printf("waking pump %s\n", PUMP_ID);
  if (!pump_wakeup()) {
    printf("wakeup failed\n");
    return -128;
  }
  model = pump_model();
  printf("model %d\n", model);
  rssi = read_rssi();
  printf("rssi %d\n", rssi);
  return rssi;
}

char str[100];
void splash() {
  oled_on();
  font_large();
  align_center_both();
  draw_string(64, 32, "MMTune");
  oled_update();
}

void app_main() {
  initArduino();
  rfm95_init();
  uint8_t v = read_version();
  printf("radio version %d.%d\n", version_major(v), version_minor(v));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  oled_init();
  splash();
  delayMicroseconds(2 * SECONDS);
  parse_pump_id(PUMP_ID);

  oled_clear();
  font_medium();
  align_left();
  draw_string(0, 0, "Scanning...");
  oled_update();
  delayMicroseconds(1 * SECONDS);

  int best_rssi = -128;
  long best_freq = 0;
  for (int i = 0; i < 11; i++) {
    long cur_freq = PUMP_FREQUENCY_START + i * 50000;
    int rssi = try_frequency(cur_freq);
    if (rssi > best_rssi) {
      best_rssi = rssi;
      best_freq = cur_freq;
    }
    draw_freq_bar(i, rssi);
    delayMicroseconds(100);
    oled_update();
    delayMicroseconds(1 * SECONDS);
  }
  printf("Best frequency %ld Hz\n", best_freq);
  printf("RSSI: %d\n", best_rssi);
  draw_string(0, 0, "Scanning... done.");
  delayMicroseconds(100);
  oled_update();
  delayMicroseconds(5 * SECONDS);

  oled_clear();
  font_medium();
  align_left();
  sprintf(str, "Best RSSI: %d", best_rssi);
  draw_string(0, 0, str);
  sprintf(str, "Best Frq: %d.%d", FP_FQ(best_freq));
  draw_string(0, 27, str);
  delayMicroseconds(100);
  oled_update();

  delayMicroseconds(DISPLAY_TIMEOUT);

  // Wake up on button press.
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
  esp_deep_sleep_start();
}
