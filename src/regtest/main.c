#include <stdint.h>
#include <stdio.h>

#include "rfm95.h"
#include "spi.h"

#define MAX_ADDR	66
#define BURST_MAX	64

void check_regs(void) {
	static uint8_t regs0[MAX_ADDR+1];
	static uint8_t regs1[MAX_ADDR+1];
	for (uint8_t addr = 1; addr <= MAX_ADDR; addr++) {
		regs0[addr] = read_register(addr);
	}
	int n = MAX_ADDR/2;
	read_burst(1, &regs1[1], n);
	read_burst(n, &regs1[n], n + ((MAX_ADDR + 1) % 2));
	int mismatches = 0;
	for (uint8_t addr = 1; addr <= MAX_ADDR; addr++) {
		if (regs0[addr] != regs1[addr]) {
			printf("%02X  %02X (single) != %02X (burst)\n", addr, regs0[addr], regs1[addr]);
			mismatches++;
		}
	}
	if (mismatches == 0) {
		printf("Burst-mode read is working correctly.\n");
	} else {
		printf("WARNING: burst read did not match %d of %d single reads\n", mismatches, MAX_ADDR);
	}
	printf("Configuration registers:\n");
	for (uint8_t addr = 1; addr <= MAX_ADDR; addr++) {
		printf("%02X  %02X\n", addr, regs0[addr]);
	}
}


void read_regs(const char *kind, uint8_t *data) {
	printf("\nTesting %s writes\n", kind);
	printf("source: %02X %02X %02X\n", data[0], data[1], data[2]);
	uint8_t x = read_register(REG_SYNC_VALUE_1);
	uint8_t y = read_register(REG_SYNC_VALUE_2);
	uint8_t z = read_register(REG_SYNC_VALUE_3);
	printf("single: %02X %02X %02X\n", x, y, z);
	uint8_t v[3];
	read_burst(REG_SYNC_VALUE_1, v, sizeof(v));
	printf(" burst: %02X %02X %02X\n", v[0], v[1], v[2]);
	if (v[0] != data[0] || v[1] != data[1] || v[2] != data[2]) {
		printf("ERROR: burst reads did not match %s writes\n", kind);
	}
}

void app_main(void) {
	spi_init();
	rfm95_reset();
	check_regs();

	static uint8_t single_data[3] = { 0x44, 0x55, 0x66 };
	write_register(REG_SYNC_VALUE_1, single_data[0]);
	write_register(REG_SYNC_VALUE_2, single_data[1]);
	write_register(REG_SYNC_VALUE_3, single_data[2]);
	read_regs("single", single_data);

	static uint8_t burst_data[3] = { 0x77, 0x88, 0x99 };
	write_burst(REG_SYNC_VALUE_1, burst_data, sizeof(burst_data));
	read_regs("burst", burst_data);

	rfm95_reset();
	check_regs();
}
