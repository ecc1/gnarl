#include <unistd.h>

#define TAG		"rfm95"

#include <esp_log.h>
#include <esp_sleep.h>
#include <esp32/rom/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "module.h"
#include "rfm95.h"
#include "spi.h"

#define MILLISECONDS	1000

static inline uint8_t read_mode() {
	return read_register(REG_OP_MODE) & OP_MODE_MASK;
}

#define MAX_WAIT	1000

static void set_mode(uint8_t mode) {
	uint8_t cur_mode = read_mode();
	if (mode == cur_mode) {
		return;
	}
	ESP_LOGD(TAG, "set_mode %d -> %d", cur_mode, mode);
	write_register(REG_OP_MODE, FSK_OOK_MODE | MODULATION_OOK | mode);
	if (cur_mode == MODE_SLEEP) {
		usleep(500);
	}
	for (int w = 0; w < MAX_WAIT; w++) {
		cur_mode = read_mode();
		if (cur_mode == mode) {
			return;
		}
	}
	ESP_LOGI(TAG, "set_mode(%d) timeout in mode %d", mode, cur_mode);
}

static inline void set_mode_sleep() {
	set_mode(MODE_SLEEP);
}

static inline void set_mode_standby() {
	set_mode(MODE_STDBY);
}

static inline void set_mode_receive() {
	set_mode(MODE_RX);
}

static inline void set_mode_transmit() {
	set_mode(MODE_TX);
}

static inline void sequencer_stop() {
	write_register(REG_SEQ_CONFIG_1, SEQUENCER_STOP);
}

void rfm95_reset() {
	ESP_LOGD(TAG, "reset");
	gpio_set_direction(LORA_RST, GPIO_MODE_OUTPUT);
	gpio_set_level(LORA_RST, 0);
	usleep(100);
	gpio_set_level(LORA_RST, 1);
	usleep(5*MILLISECONDS);
}

static volatile int rx_packets;

int rx_packet_count() {
	return rx_packets;
}

static volatile int tx_packets;

int tx_packet_count() {
	return tx_packets;
}

static volatile TaskHandle_t rx_waiting_task;

static void IRAM_ATTR rx_interrupt() {
	rx_packets++;
	if (rx_waiting_task != 0) {
		vTaskNotifyGiveFromISR(rx_waiting_task, 0);
	}
}

void rfm95_init() {
	spi_init();
	rfm95_reset();

	gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

	gpio_set_direction(DIO2, GPIO_MODE_INPUT);
	gpio_set_intr_type(DIO2, GPIO_INTR_POSEDGE);
	gpio_isr_handler_add(DIO2, rx_interrupt, 0);
	// Interrupt on DIO2 when SyncMatch occurs.
	write_register(REG_DIO_MAPPING_1, 3 << DIO2_MAPPING_SHIFT);
	// Wake up on receive interrupt.
	gpio_wakeup_enable(DIO2, GPIO_INTR_HIGH_LEVEL);
	esp_sleep_enable_gpio_wakeup();

	// Must be in Sleep mode first before the second call can change to FSK/OOK mode.
	set_mode_sleep();
	set_mode_sleep();

	// Ideal bit rate is 16384 bps; this works out to 16385 bps.
	write_register(REG_BITRATE_MSB, 0x07);
	write_register(REG_BITRATE_LSB, 0xA1);

	// Use 64 samples for RSSI.
	write_register(REG_RSSI_CONFIG, 5);

	// 200 kHz channel bandwidth (mantissa = 20, exp = 1)
	write_register(REG_RX_BW, (1 << RX_BW_MANT_SHIFT) | 1);

	// Make sure enough preamble bytes are sent.
	write_register(REG_PREAMBLE_MSB, 0x00);
	write_register(REG_PREAMBLE_LSB, 0x18);

	// Use 4 bytes for Sync word.
	write_register(REG_SYNC_CONFIG, SYNC_ON | 3);

	// Sync word.
	write_register(REG_SYNC_VALUE_1, 0xFF);
	write_register(REG_SYNC_VALUE_2, 0x00);
	write_register(REG_SYNC_VALUE_3, 0xFF);
	write_register(REG_SYNC_VALUE_4, 0x00);

	write_register(REG_PACKET_CONFIG_1, PACKET_FORMAT_FIXED);
	write_register(REG_PACKET_CONFIG_2, PACKET_MODE);
}

static inline bool fifo_empty() {
	return (read_register(REG_IRQ_FLAGS_2) & FIFO_EMPTY) != 0;
}

static inline bool fifo_full() {
	return (read_register(REG_IRQ_FLAGS_2) & FIFO_FULL) != 0;
}

static inline void clear_fifo() {
	write_register(REG_IRQ_FLAGS_2, FIFO_OVERRUN);
}

static inline uint8_t read_fifo_flags() {
	return read_register(REG_IRQ_FLAGS_2);
}

static inline void xmit_byte(uint8_t b) {
	write_register(REG_FIFO, b);
}

static inline void xmit(uint8_t* data, int len) {
	write_burst(REG_FIFO, data, len);
}

static bool wait_for_fifo_non_empty() {
	for (int w = 0; w < MAX_WAIT; w++) {
		if (!fifo_full()) {
			return true;
		}
	}
	sequencer_stop();
	set_mode_sleep();
	ESP_LOGI(TAG, "FIFO still full; flags = %02X", read_fifo_flags());
	return false;
}

static void wait_for_transmit_done() {
	uint8_t mode;
	for (int w = 0; w < MAX_WAIT; w++) {
		mode = read_mode();
		if (mode == MODE_STDBY) {
			ESP_LOGD(TAG, "transmit done; waits = %d", w);
			return;
		}
	}
	sequencer_stop();
	set_mode_sleep();
	ESP_LOGI(TAG, "transmit still not done; mode = %d", mode);
}

void transmit(uint8_t *buf, int count) {
	ESP_LOGD(TAG, "transmit %d-byte packet", count);
	set_mode_standby();
	clear_fifo();
	// Automatically enter Transmit state on FifoLevel interrupt.
	write_register(REG_FIFO_THRESH, TX_START_CONDITION);
	write_register(REG_SEQ_CONFIG_1,
		       SEQUENCER_START |
		       IDLE_MODE_SLEEP |
		       FROM_START_TO_TX_ON_FIFO_LEVEL |
		       LOW_POWER_SELECT_OFF |
		       FROM_TX_TO_LOW_POWER);
	// Specify fixed length packet format (including final zero byte)
	// so PacketSent interrupt will terminate Transmit state.
	write_register(REG_PACKET_CONFIG_1, PACKET_FORMAT_FIXED);
	uint8_t length_msb = ((count + 1) >> 8) & PAYLOAD_LENGTH_MSB_MASK;
	write_register(REG_PACKET_CONFIG_2, PACKET_MODE | length_msb);
	write_register(REG_PAYLOAD_LENGTH, (count + 1) & 0xFF);
	int n = count < FIFO_SIZE ? count : FIFO_SIZE;
	xmit(buf, n);
	ESP_LOGD(TAG, "after xmit: mode = %d", read_mode());
	while (n < count) {
		if (!wait_for_fifo_non_empty()) return;
		xmit_byte(buf[n]);
		n++;
	}
	if (!wait_for_fifo_non_empty()) return;
	xmit_byte(0);
	// Rely on the sequencer to end Transmit mode after PacketSent is triggered.
	wait_for_transmit_done();
	set_mode_sleep();
	tx_packets++;
}

static bool packet_seen() {
	bool seen = (read_register(REG_IRQ_FLAGS_1) & SYNC_ADDRESS_MATCH) != 0;
	if (seen) {
		ESP_LOGD(TAG, "incoming packet seen");
	}
	return seen;
}

static inline uint8_t recv_byte() {
	return read_register(REG_FIFO);
}

static uint8_t last_rssi = 0xFF;

int read_rssi() {
	return -(int)last_rssi / 2;
}

typedef void wait_fn_t(int);

static int rx_common(wait_fn_t wait_fn, uint8_t *buf, int count, int timeout) {
	// Use unlimited length packet format (data sheet section 4.2.13.2).
	write_register(REG_PACKET_CONFIG_1, PACKET_FORMAT_FIXED);
	write_register(REG_PAYLOAD_LENGTH, 0);
	gpio_intr_enable(DIO2);
	ESP_LOGD(TAG, "starting receive");
	set_mode_receive();
	if (!packet_seen()) {
		// Stay in RX mode.
		wait_fn(timeout);
		if (!packet_seen()) {
			set_mode_sleep();
			ESP_LOGD(TAG, "receive timeout");
			return 0;
		}
	}
	last_rssi = read_register(REG_RSSI);
	int n = 0;
	int w = 0;
	while (n < count) {
		if (fifo_empty()) {
			usleep(500);
			w++;
			if (w >= MAX_WAIT) {
				ESP_LOGD(TAG, "max RX FIFO wait reached");
				break;
			}
			continue;
		}
		uint8_t b = recv_byte();
		if (b == 0) {
			break;
		}
		buf[n++] = b;
		w = 0;
	}
	set_mode_sleep();
	clear_fifo();
	gpio_intr_disable(DIO2);
	if (n > 0) {
		// Remove spurious final byte consisting of just one or two high bits.
		uint8_t b = buf[n-1];
		if (b == 0x80 || b == 0xC0) {
			ESP_LOGD(TAG, "end-of-packet glitch %X with RSSI %d", b >> 6, read_rssi());
			n--;
		}
	}
	return n;
}

static void sleep_until_interrupt(int timeout) {
	uart_tx_wait_idle(0);
	uint64_t us = (uint64_t)timeout * MILLISECONDS;
	esp_sleep_enable_timer_wakeup(us);
	esp_light_sleep_start();
	esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
}

int sleep_receive(uint8_t *buf, int count, int timeout) {
	return rx_common(sleep_until_interrupt, buf, count, timeout);
}

#ifdef USE_POLLING

#define POLL_INTERVAL	5  // milliseconds

static void wait_until_interrupt(int timeout) {
	while (!packet_seen() && timeout > 0) {
		int t = timeout < POLL_INTERVAL ? timeout : POLL_INTERVAL;
		usleep(t * MILLISECONDS);
		timeout -= t;
	}
}

#else

static void wait_until_interrupt(int timeout) {
	ESP_LOGD(TAG, "waiting until interrupt");
	rx_waiting_task = xTaskGetCurrentTaskHandle();
	xTaskNotifyWait(0, 0, 0, pdMS_TO_TICKS(timeout));
	rx_waiting_task = 0;
	ESP_LOGD(TAG, "finished waiting");
}

#endif

int receive(uint8_t *buf, int count, int timeout) {
	return rx_common(wait_until_interrupt, buf, count, timeout);
}

uint32_t read_frequency() {
	uint8_t frf[3];
	read_burst(REG_FRF_MSB, frf, sizeof(frf));
	uint32_t f = (frf[0] << 16) | (frf[1] << 8) | frf[2];
	return ((uint64_t)f * FXOSC) >> 19;
}

void set_frequency(uint32_t freq_hz) {
	uint32_t f = (((uint64_t)freq_hz << 19) + FXOSC/2) / FXOSC;
	uint8_t frf[3];
	frf[0] = f >> 16;
	frf[1] = f >> 8;
	frf[2] = f;
	write_burst(REG_FRF_MSB, frf, sizeof(frf));
}

int read_version() {
	return read_register(REG_VERSION);
}

int version_major(int v) {
	return v >> 4;
}

int version_minor(int v) {
	return v & 0xF;
}
