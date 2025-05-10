#include "shakal_cam.h"

void sendNewBMP(camera_fb_t *fb, WiFiClient &client) {
  const int width = fb->width;
  const int height = fb->height;
  const int rowSize = (width + 3) & ~3; // BMP rows are padded to multiple of 4
  const int fileSize = 54 + 1024 + rowSize * height; // 54 bytes header + 1024 bytes palette + pixel data

  // --- BMP Header (54 bytes) ---
  uint8_t header[54] = {
    'B', 'M',
    (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
    0, 0, 0, 0,
    (uint8_t)(54 + 1024), (uint8_t)((54 + 1024) >> 8), (uint8_t)((54 + 1024) >> 16), (uint8_t)((54 + 1024) >> 24), // offset to pixel data
    40, 0, 0, 0,        // DIB header size
    (uint8_t)(width), (uint8_t)(width >> 8), (uint8_t)(width >> 16), (uint8_t)(width >> 24),
    (uint8_t)(height), (uint8_t)(height >> 8), (uint8_t)(height >> 16), (uint8_t)(height >> 24),
    1, 0,               // planes
    8, 0,               // bits per pixel
    0, 0, 0, 0,         // compression = none
    0, 0, 0, 0,         // image size (can be 0 for no compression)
    0x13, 0x0B, 0, 0,   // X pixels per meter (2835 = 72 DPI)
    0x13, 0x0B, 0, 0,   // Y pixels per meter
    (uint8_t)(256), (uint8_t)(256 >> 8), (uint8_t)(256 >> 16), (uint8_t)(256 >> 24), // colors used
    0, 0, 0, 0          // important colors
  };

  // --- Send header ---
  client.write(header, 54);

  // --- Grayscale palette (256 * 4 bytes) ---
  for (int i = 0; i < 256; i++) {
    client.write((uint8_t)i); // B
    client.write((uint8_t)i); // G
    client.write((uint8_t)i); // R
    client.write((uint8_t)0); // Reserved
  }

  // --- Image data (bottom to top) ---
  for (int y = height - 1; y >= 0; y--) {
    size_t offset = y * width;
    client.write(fb->buf + offset, width);          // grayscale row
    for (int pad = width; pad < rowSize; pad++) {
      client.write((uint8_t)0);                    // padding
    }
  }
}
