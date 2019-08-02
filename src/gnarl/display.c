#include "gnarl.h"

#include <string.h>
#include <unistd.h>

#include <esp_timer.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "display.h"
#include "module.h"
#include "oled.h"

typedef struct {
	display_op_t op;
	int arg;
} display_command_t;

#define QUEUE_LENGTH	100

static QueueHandle_t display_queue;

static int connected = false;
static int phone_rssi;
static int pump_rssi;
static int command_time;  // seconds

#define DISPLAY_TIMEOUT	5  // seconds

static void format_time_ago(char *buf) {
	int now = esp_timer_get_time() / 1000000;
	int delta = now - command_time;
	int min = delta / 60;
	if (min == 0) {
		sprintf(buf, "%ds", delta);
		return;
	}
	if (min < 60) {
		sprintf(buf, "%dm", min);
		return;
	}
	int hr = min / 60;
	min = min % 60;
	sprintf(buf, "%dh%dm", hr, min);
}

static void update(display_command_t cmd) {
	switch (cmd.op) {
	case PHONE_RSSI:
		phone_rssi = cmd.arg;
		ESP_LOGD(TAG, "phone RSSI = %d", phone_rssi);
		return;
	case PUMP_RSSI:
		pump_rssi = cmd.arg;
		ESP_LOGD(TAG, "pump RSSI = %d", pump_rssi);
		return;
	case COMMAND_TIME:
		command_time = cmd.arg;
		ESP_LOGD(TAG, "command time = %d", command_time);
		return;
	case CONNECTED:
		connected = cmd.arg;
		break;
	default:
		break;
	}
	oled_on();
	oled_clear();

	oled_font_medium();
        oled_align_center();
        oled_draw_string(64, 15, connected ? "Connected" : "Disconnected");

	oled_font_small();
	oled_align_left();
        oled_draw_string(5, 32, "Last command:");
        oled_draw_string(5, 46, "Phone RSSI:");
        oled_draw_string(5, 60, "Pump  RSSI:");

	oled_align_right();
	char buf[16];
	format_time_ago(buf);
	oled_draw_string(122, 32, buf);
	if (connected) {
		sprintf(buf, "%d", phone_rssi);
		oled_draw_string(122, 46, buf);
		sprintf(buf, "%d", pump_rssi);
		oled_draw_string(122, 60, buf);
	} else {
		oled_draw_string(122, 46, "--");
		oled_draw_string(122, 60, "--");
	}

        oled_update();
	usleep(DISPLAY_TIMEOUT*SECONDS);
	oled_off();
}

static void display_loop() {
	for (;;) {
		display_command_t cmd;
		if (!xQueueReceive(display_queue, &cmd, pdMS_TO_TICKS(100))) {
			continue;
		}
		ESP_LOGD(TAG, "display_loop: op %d arg %d", cmd.op, cmd.arg);
		update(cmd);
	}
}

static void button_interrupt() {
	display_command_t cmd = { .op = SHOW_STATUS };
	xQueueSendFromISR(display_queue, &cmd, 0);
}

void display_update(display_op_t op, int arg) {
	display_command_t cmd = { .op = op, .arg = arg };
	if (!xQueueSend(display_queue, &cmd, 0)) {
		ESP_LOGE(TAG, "display_update: queue full");
	}
}

void display_init(void) {
	oled_init();
	display_queue = xQueueCreate(QUEUE_LENGTH, sizeof(display_command_t));
	xTaskCreate(display_loop, "display", 2048, 0, 10, 0);
	display_update(SHOW_STATUS, 0);

	// Enable interrupt on button press.
	gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
	gpio_set_intr_type(BUTTON, GPIO_INTR_NEGEDGE);
	gpio_isr_handler_add(BUTTON, button_interrupt, 0);
	gpio_intr_enable(BUTTON);
}
