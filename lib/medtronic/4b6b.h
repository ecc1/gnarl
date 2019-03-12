#ifndef _4B6B_H
#define _4B6B_H

#include <stddef.h>
#include <stdint.h>

// Encode bytes using 4b/6b encoding.
// Encoding n bytes produces 3 * (n / 2) + 2 * (n % 2) output bytes.
// Return number of bytes written to dst.

int encode_4b6b(const uint8_t *src, uint8_t *dst, size_t len);

// Decode bytes using 4b/6b encoding.
// Decoding n bytes produces 2 * (n / 3) + (n % 3) / 2 output bytes.
// Return number of bytes written to dst if successful,
// or -1 if invalid input was encountered.

int decode_4b6b(const uint8_t *src, uint8_t *dst, size_t len);

#endif /* _4B6B_H */
