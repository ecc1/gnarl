#include <stdint.h>
#include <stdio.h>

#include "rfm95.h"
#include "spi.h"

void read_regs() {
	uint8_t x = read_register(REG_SYNC_VALUE_1);
	uint8_t y = read_register(REG_SYNC_VALUE_2);
	uint8_t z = read_register(REG_SYNC_VALUE_3);
	printf("single: %02X %02X %02X\n", x, y, z);
	uint8_t data[3];
	read_burst(REG_SYNC_VALUE_1, data, sizeof(data));
	printf(" burst: %02X %02X %02X\n", data[0], data[1], data[2]);
}

void app_main() {
	spi_init();

	for (uint8_t addr = 0; addr <= 0x50; addr++) {
		printf("%02X: %02X\n", addr, read_register(addr));
	}

	printf("\nTesting individual writes\n");
	printf("source: %02X %02X %02X\n", 0x44, 0x55, 0x66);
	write_register(REG_SYNC_VALUE_1, 0x44);
	write_register(REG_SYNC_VALUE_2, 0x55);
	write_register(REG_SYNC_VALUE_3, 0x66);
	read_regs();

	printf("\nTesting burst writes\n");
	uint8_t data[] = {0x77, 0x88, 0x99};
	printf("source: %02X %02X %02X\n", data[0], data[1], data[2]);
	write_burst(REG_SYNC_VALUE_1, data, sizeof(data));
	read_regs();
}
