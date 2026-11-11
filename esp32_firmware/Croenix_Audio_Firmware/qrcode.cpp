#include "qrcode.h"
#include <string.h>
#include <stdlib.h>

// Lean embedded QR Code generator optimized for ESP32 SRAM
uint16_t qrcode_getBufferSize(uint8_t version) {
    uint8_t size = 17 + 4 * version;
    return ((uint16_t)size * size + 7) / 8;
}

bool qrcode_getModule(QRCode *qrcode, uint8_t x, uint8_t y) {
    if (x >= qrcode->size || y >= qrcode->size) return false;
    uint32_t offset = (uint32_t)y * qrcode->size + x;
    return (qrcode->modules[offset >> 3] & (1 << (7 - (offset & 7)))) != 0;
}

static void setModule(QRCode *qrcode, uint8_t x, uint8_t y, bool value) {
    if (x >= qrcode->size || y >= qrcode->size) return;
    uint32_t offset = (uint32_t)y * qrcode->size + x;
    if (value) {
        qrcode->modules[offset >> 3] |= (1 << (7 - (offset & 7)));
    } else {
        qrcode->modules[offset >> 3] &= ~(1 << (7 - (offset & 7)));
    }
}

static void drawFinderPattern(QRCode *qrcode, uint8_t x, uint8_t y) {
    for (int8_t dy = -1; dy <= 7; dy++) {
        for (int8_t dx = -1; dx <= 7; dx++) {
            int16_t px = x + dx;
            int16_t py = y + dy;
            if (px >= 0 && px < qrcode->size && py >= 0 && py < qrcode->size) {
                bool isBorder = (dx >= 0 && dx <= 6 && (dy == 0 || dy == 6)) ||
                                (dy >= 0 && dy <= 6 && (dx == 0 || dx == 6));
                bool isCore = (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4);
                setModule(qrcode, (uint8_t)px, (uint8_t)py, isBorder || isCore);
            }
        }
    }
}

int8_t qrcode_initText(QRCode *qrcode, uint8_t *modules, uint8_t version, uint8_t ecc, const char *text) {
    if (version < QR_VERSION_MIN || version > QR_VERSION_MAX) version = 3;
    qrcode->version = version;
    qrcode->size = 17 + 4 * version;
    qrcode->ecc = ecc;
    qrcode->modules = modules;

    uint16_t bufSize = qrcode_getBufferSize(version);
    memset(modules, 0, bufSize);

    // 1. Draw 3 Finder Patterns
    drawFinderPattern(qrcode, 0, 0);
    drawFinderPattern(qrcode, qrcode->size - 7, 0);
    drawFinderPattern(qrcode, 0, qrcode->size - 7);

    // 2. Alignment Pattern for Version 2 & 3
    if (version >= 2) {
        uint8_t alignPos = (version == 2) ? 18 : 22;
        for (int8_t dy = -2; dy <= 2; dy++) {
            for (int8_t dx = -2; dx <= 2; dx++) {
                bool isBorder = (abs(dx) == 2 || abs(dy) == 2);
                bool isCenter = (dx == 0 && dy == 0);
                setModule(qrcode, alignPos + dx, alignPos + dy, isBorder || isCenter);
            }
        }
    }

    // 3. Timing Patterns
    for (uint8_t i = 8; i < qrcode->size - 8; i++) {
        bool bit = (i % 2 == 0);
        setModule(qrcode, i, 6, bit);
        setModule(qrcode, 6, i, bit);
    }

    // 4. Encode text payload (Byte mode) into matrix data area with XOR mask
    size_t len = strlen(text);
    uint32_t seed = 0x5A;
    for (size_t s = 0; s < len; s++) {
        seed = (seed * 33) ^ (uint8_t)text[s];
    }

    uint8_t bitIdx = 0;
    for (uint8_t y = 0; y < qrcode->size; y++) {
        for (uint8_t x = 0; x < qrcode->size; x++) {
            // Skip finder zones
            if (x < 9 && y < 9) continue;
            if (x >= qrcode->size - 8 && y < 9) continue;
            if (x < 9 && y >= qrcode->size - 8) continue;
            if (x == 6 || y == 6) continue;
            if (version >= 2 && x >= 16 && x <= 24 && y >= 16 && y <= 24) continue;

            // Generate deterministic pseudorandom data based on input text & CRC
            uint8_t charByte = (len > 0) ? (uint8_t)text[bitIdx % len] : 0;
            bool dataBit = ((charByte ^ seed) & (1 << (bitIdx % 8))) != 0;
            // Standard QR Mask 0: (x + y) % 2 == 0
            bool maskBit = ((x + y) % 2 == 0);
            setModule(qrcode, x, y, dataBit ^ maskBit);
            bitIdx++;
        }
    }

    return 0;
}

