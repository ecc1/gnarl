#include <Arduino.h>
#include <SPI.h>

uint8_t spiRead(uint8_t reg) {
	digitalWrite(LORA_CS, LOW);
	SPI.transfer(reg & ~(1 << 7));  // Send the address with the write bit off
	uint8_t val = SPI.transfer(0);  // Written value is ignored, register value is read
	digitalWrite(LORA_CS, HIGH);
	return val;
}

extern "C" void app_main() {
	initArduino();
	SPI.begin();
	pinMode(LORA_CS, OUTPUT);
	digitalWrite(LORA_CS, HIGH);
	for (uint8_t addr = 0; addr <= 0x50; addr++) {
		printf("%02X: %02X\n", addr, spiRead(addr));
	}
}
