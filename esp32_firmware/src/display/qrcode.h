#ifndef QRCODE_H
#define QRCODE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// QR Code Error Correction Level
#define ECC_LOW      0
#define ECC_MEDIUM   1
#define ECC_QUARTILE 2
#define ECC_HIGH     3

// Maximum supported version for embedded memory constraint (Version 3: 29x29)
#define QR_VERSION_MIN 1
#define QR_VERSION_MAX 3

typedef struct {
    uint8_t version;
    uint8_t size;
    uint8_t ecc;
    uint8_t mode;
    uint8_t mask;
    uint8_t *modules;
} QRCode;

// Buffer size calculator: ((size * size) + 7) / 8
// Version 1 (21x21): 56 bytes
// Version 2 (25x25): 79 bytes
// Version 3 (29x29): 106 bytes
uint16_t qrcode_getBufferSize(uint8_t version);

int8_t qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version, uint8_t ecc, const char *text);

bool qrcode_getModule(QRCode *qrcode, uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

#endif // QRCODE_H
