#ifndef _CRC_H
#define _CRC_H

#include <stddef.h>
#include <stdint.h>

// CRC-8 using polyomial 0x9B (WCDMA variant).

uint8_t crc8(const uint8_t *buf, size_t len);

// CRC-16 using polyomial 0x1021 (CCITT variant).

uint16_t crc16(const uint8_t *buf, size_t len);

#endif /* _CRC_H */
