#include "shakal_cam.h"

void sendNewBMP(camera_fb_t *fb, WiFiClient &client, bool use8ColorPalette) {
  const int width = fb->width;
  const int height = fb->height;
  const int rowSize = (width + 3) & ~3; // BMP rows are padded to multiple of 4
  const int paletteSize = use8ColorPalette ? 8 : 256;
  const int paletteBytes = paletteSize * 4;
  const int fileSize = 54 + paletteBytes + rowSize * height; // header + palette + pixel data

  // --- BMP Header (54 bytes) ---
  uint8_t header[54] = {
    'B', 'M',
    (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
    0, 0, 0, 0,
    (uint8_t)(54 + paletteBytes), (uint8_t)((54 + paletteBytes) >> 8), (uint8_t)((54 + paletteBytes) >> 16), (uint8_t)((54 + paletteBytes) >> 24), // offset to pixel data
    40, 0, 0, 0,        // DIB header size
    (uint8_t)(width), (uint8_t)(width >> 8), (uint8_t)(width >> 16), (uint8_t)(width >> 24),
    (uint8_t)(height), (uint8_t)(height >> 8), (uint8_t)(height >> 16), (uint8_t)(height >> 24),
    1, 0,               // planes
    8, 0,               // bits per pixel
    0, 0, 0, 0,         // compression = none
    0, 0, 0, 0,         // image size (can be 0 for no compression)
    0x13, 0x0B, 0, 0,   // X pixels per meter (2835 = 72 DPI)
    0x13, 0x0B, 0, 0,   // Y pixels per meter
    (uint8_t)(paletteSize), (uint8_t)(paletteSize >> 8), (uint8_t)(paletteSize >> 16), (uint8_t)(paletteSize >> 24), // colors used
    0, 0, 0, 0          // important colors
  };

  // --- Send header ---
  client.write(header, 54);

  if (use8ColorPalette) {
    // --- 8-color palette (BGR0 order) ---
    uint8_t palette[8][4] = {
      {0,   0,   0,   0},   // Black
      {0,   0, 255,   0},   // Red
      {0, 255,   0,   0},   // Green
      {255, 0,   0,   0},   // Blue
      {0, 255, 255, 0},     // Yellow
      {255,255,  0,  0},    // Cyan
      {255, 0, 255, 0},     // Magenta
      {255,255,255, 0}      // White
    };
    for (int i = 0; i < 8; i++) {
      client.write(palette[i], 4);
    }
  } else {
    // --- Grayscale palette (256 * 4 bytes) ---
    for (int i = 0; i < 256; i++) {
      client.write((uint8_t)i); // B
      client.write((uint8_t)i); // G
      client.write((uint8_t)i); // R
      client.write((uint8_t)0); // Reserved
    }
  }

  // --- Big pixel buffer allocation and processing ---
  uint8_t* bigPixelBuf = (uint8_t*)malloc(width * height);
  if (bigPixelBuf) {
    makeBigPixels8x8(fb->buf, width, height, bigPixelBuf);
  } else {
    bigPixelBuf = fb->buf;
  }

  // --- Image data (bottom to top) ---
  for (int y = height - 1; y >= 0; y--) {
    size_t offset = y * width;
    if (use8ColorPalette) {
      // Map grayscale to 8-color palette using thresholds
      // 0-31: black, 32-63: red, 64-95: green, 96-127: blue, 128-159: yellow, 160-191: cyan, 192-223: magenta, 224-255: white
      for (int x = 0; x < width; x++) {
        uint8_t v = bigPixelBuf[offset + x];
        uint8_t idx;
        if (v < 32) idx = 0;         // Black
        else if (v < 64) idx = 1;    // Red
        else if (v < 96) idx = 2;    // Green
        else if (v < 128) idx = 3;   // Blue
        else if (v < 160) idx = 4;   // Yellow
        else if (v < 192) idx = 5;   // Cyan
        else if (v < 224) idx = 6;   // Magenta
        else idx = 7;                // White
        client.write(&idx, 1);
      }
    } else {
      client.write(bigPixelBuf + offset, width); // grayscale row
    }
    for (int pad = width; pad < rowSize; pad++) {
      client.write((uint8_t)0); // padding
    }
  }

  if (bigPixelBuf != fb->buf) {
    free(bigPixelBuf);
  }
}

// Create a new buffer with 8x8 big pixels (pixelation effect)
// src: pointer to original grayscale buffer
// srcWidth, srcHeight: dimensions of the original image
// dst: pointer to output buffer (must be preallocated with (srcWidth/8*8)*(srcHeight/8*8) bytes)
void makeBigPixels8x8(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst) {
    int blockSize = 8;
    int dstWidth = (srcWidth / blockSize) * blockSize;
    int dstHeight = (srcHeight / blockSize) * blockSize;
    for (int by = 0; by < dstHeight; by += blockSize) {
        for (int bx = 0; bx < dstWidth; bx += blockSize) {
            // Compute average for this 8x8 block
            int sum = 0;
            for (int y = 0; y < blockSize; ++y) {
                for (int x = 0; x < blockSize; ++x) {
                    sum += src[(by + y) * srcWidth + (bx + x)];
                }
            }
            uint8_t avg = sum / (blockSize * blockSize);
            // Fill 8x8 block in dst with avg
            for (int y = 0; y < blockSize; ++y) {
                for (int x = 0; x < blockSize; ++x) {
                    dst[(by + y) * dstWidth + (bx + x)] = avg;
                }
            }
        }
    }
}
