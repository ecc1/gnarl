#include <driver/gpio.h>
#include <driver/i2c.h>
#include <u8g2.h>

#include "module.h"

static i2c_cmd_handle_t handle_i2c;

#define I2C_MASTER_NUM		I2C_NUM_1
#define I2C_MASTER_FREQ_HZ      400000
#define I2C_TIMEOUT_MS		1000

static void oled_i2c_init(void) {
	i2c_config_t conf = {
		.mode		  = I2C_MODE_MASTER,
		.sda_io_num	  = OLED_SDA,
		.sda_pullup_en	  = GPIO_PULLUP_ENABLE,
		.scl_io_num	  = OLED_SCL,
		.scl_pullup_en	  = GPIO_PULLUP_ENABLE,
		.master.clk_speed = I2C_MASTER_FREQ_HZ,
	};
	ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}

uint8_t i2c_callback(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch(msg) {
	case U8X8_MSG_BYTE_INIT:
		oled_i2c_init();
		break;
	case U8X8_MSG_BYTE_SEND: {
		uint8_t *p = (uint8_t *)arg_ptr;
		// Note: attempts to replace this loop with a single call to i2c_master_write
		// did not work (no error, but nothing displayed) even at lower I2C frequencies.
		while (arg_int > 0) {
			ESP_ERROR_CHECK(i2c_master_write_byte(handle_i2c, *p, true));
			p++;
			arg_int--;
		}
		break;
	}
	case U8X8_MSG_BYTE_START_TRANSFER:
		handle_i2c = i2c_cmd_link_create();
		ESP_ERROR_CHECK(i2c_master_start(handle_i2c));
		ESP_ERROR_CHECK(i2c_master_write_byte(handle_i2c, u8x8_GetI2CAddress(u8x8) | I2C_MASTER_WRITE, true));
		break;
	case U8X8_MSG_BYTE_END_TRANSFER:
		ESP_ERROR_CHECK(i2c_master_stop(handle_i2c));
		ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, handle_i2c, I2C_TIMEOUT_MS / portTICK_RATE_MS));
		i2c_cmd_link_delete(handle_i2c);
		break;
	default:
		return 0;
	}
	return 1;
}

static void oled_gpio_init(void) {
	gpio_config_t conf = {
		.pin_bit_mask = 1 << OLED_RST,
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK(gpio_config(&conf));
}

uint8_t gpio_and_delay_callback(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch (msg) {
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		oled_gpio_init();
		break;
	case U8X8_MSG_GPIO_RESET:
		gpio_set_level(OLED_RST, arg_int);
		break;
	case U8X8_MSG_GPIO_CS:
		break;
	case U8X8_MSG_GPIO_I2C_CLOCK:
		gpio_set_level(OLED_SCL, arg_int);
		break;
	case U8X8_MSG_GPIO_I2C_DATA:
		gpio_set_level(OLED_SDA, arg_int);
		break;
	case U8X8_MSG_DELAY_MILLI:
		vTaskDelay(arg_int / portTICK_PERIOD_MS);
		break;
	}
	return 1;
}
