#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>

#define MEGAHERTZ		1000000

#define SPI_WRITE		(1 << 7)

void spi_init(void);

uint8_t read_register(uint8_t addr);

void read_burst(uint8_t addr, uint8_t *buf, int count);

void write_register(uint8_t addr, uint8_t value);

void write_burst(uint8_t addr, uint8_t *buf, int count);

#endif // _SPI_H
