#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "gnarl.h"
#include "oled.h"

typedef struct {
	display_op_t op;
	int arg;
} display_command_t;

#define QUEUE_LENGTH	100

static QueueHandle_t display_queue;

static int connected = false;
static int pump_rssi;
static int command_time;  // seconds

#define SECONDS		1000000
#define DISPLAY_TIMEOUT	5  // seconds

static void format_time_ago(char *buf) {
	int now = esp_timer_get_time() / 1000000;
	int delta = now - command_time;
	int min = delta / 60;
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

	font_medium();
        align_center();
        draw_string(64, 10, connected ? "Connected" : "Disonnected");

	font_small();
	align_left();
        draw_string(5, 32, "Last command:");
        draw_string(5, 45, "Pump RSSI:");
	align_right();
	char buf[16];
	format_time_ago(buf);
        draw_string(122, 32, buf);
	sprintf(buf, "%d", pump_rssi);
        draw_string(122, 45, buf);

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
	assert(display_queue != 0);
	xTaskCreate(display_loop, "display", 2048, 0, 10, 0);
	display_update(SHOW_STATUS, 0);

	// Enable interrupt on button press.
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_NEGEDGE);
	gpio_isr_handler_add(GPIO_NUM_0, button_interrupt, 0);
	gpio_intr_enable(GPIO_NUM_0);
}
