#include <stdint.h>
#include <stdio.h>

#include "spi.h"

void app_main() {
	spi_init();
	for (uint8_t addr = 0; addr <= 0x50; addr++) {
		printf("%02X: %02X\n", addr, read_register(addr));
	}
}
