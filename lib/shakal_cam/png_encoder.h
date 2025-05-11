#ifndef PNG_ENCODER_H
#define PNG_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include <WiFi.h>

// Encodes an 8-bit grayscale image to PNG and writes to a WiFiClient
// Returns true on success
bool encodePNGGray8ToClient(const uint8_t* image, int width, int height, WiFiClient& client);

// Encodes an 8-color indexed image to PNG and writes to a WiFiClient
// image: buffer of palette indices (0..paletteSize-1)
// palette: array of paletteSize*4 bytes (RGBA)
bool encodePNGPalette8ToClient(const uint8_t* image, int width, int height, const uint8_t* palette, int paletteSize, WiFiClient& client);

// Encodes an 8-bit grayscale image to PNG and writes to a buffer
// Returns the number of bytes written, or 0 on error
size_t encodePNGGray8ToBuffer(const uint8_t* image, int width, int height, uint8_t* outBuf, size_t outBufSize);

#endif // PNG_ENCODER_H
