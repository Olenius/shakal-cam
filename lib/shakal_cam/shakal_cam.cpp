#include "shakal_cam.h"

// Reusable buffer for big pixels (allocated once, reused)
static uint8_t* g_bigPixelBuf = nullptr;
static size_t g_bigPixelBufSize = 0;

void ensureBigPixelBuf(size_t size) {
    if (!g_bigPixelBuf || g_bigPixelBufSize < size) {
        if (g_bigPixelBuf) free(g_bigPixelBuf);
        g_bigPixelBuf = (uint8_t*)malloc(size);
        g_bigPixelBufSize = size;
    }
}

// Helper: crop center 3:2 region from grayscale buffer
// src: pointer to original buffer (width x height)
// dst: pointer to output buffer (width x cropHeight)
// width: image width (320)
// height: image height (240)
// cropHeight: cropped height (213)
void crop_center_3_2(const uint8_t* src, uint8_t* dst, int width, int height, int cropHeight) {
    int y0 = (height - cropHeight) / 2;
    for (int y = 0; y < cropHeight; ++y) {
        memcpy(dst + y * width, src + (y0 + y) * width, width);
    }
}

void sendNewBMP(camera_fb_t *fb, WiFiClient &client, bool use8ColorPalette) {
  int width = fb->width;
  int height = fb->height;
  bool crop3_2 = (width == 320 && height == 240); // QVGA only
  int cropHeight = crop3_2 ? 213 : height;
  int rowSize = (width + 3) & ~3; // BMP rows are padded to multiple of 4
  int paletteSize = use8ColorPalette ? 8 : 256;
  int paletteBytes = paletteSize * 4;
  int fileSize = 54 + paletteBytes + rowSize * cropHeight; // header + palette + pixel data

  // --- BMP Header (54 bytes) ---
  uint8_t header[54] = {
    'B', 'M',
    (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
    0, 0, 0, 0,
    (uint8_t)(54 + paletteBytes), (uint8_t)((54 + paletteBytes) >> 8), (uint8_t)((54 + paletteBytes) >> 16), (uint8_t)((54 + paletteBytes) >> 24), // offset to pixel data
    40, 0, 0, 0,        // DIB header size
    (uint8_t)(width), (uint8_t)(width >> 8), (uint8_t)(width >> 16), (uint8_t)(width >> 24),
    (uint8_t)(cropHeight), (uint8_t)(cropHeight >> 8), (uint8_t)(cropHeight >> 16), (uint8_t)(cropHeight >> 24),
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
    uint8_t palette[8][4] = {
      // {18, 18, 18, 0},     // Очень тёмно-серый (почти чёрный)
      // {45, 45, 48, 0},     // Тёмно-серый с холодным оттенком
      // {75, 75, 78, 0},     // Глубокий графитовый
      // {110, 110, 115, 0},  // Мягкий серый
      // {145, 145, 150, 0},  // Серо-стальной
      // {180, 180, 185, 0},  // Светло-серый с лёгким синим
      // {220, 220, 225, 0},  // Почти белый, но с глубиной
      // {255, 255, 255, 0}   // Чисто белый

      // {100, 70, 160, 0},    // Глубокий «плёночный» фиолетовый  
      // {130, 90, 190, 0},    // Тёплый винтажный фиолетово-синий  
      // {160, 110, 215, 0},   // Плавный приглушённый фиолетовый  
      // {190, 140, 224, 0},   // Фейдовый сиреневый  
      // {220, 170, 222, 0},   // Лавандовый с налётом пыли  
      // {239, 200, 217, 0},   // Светлый винтажный розовый  
      // {247, 225, 211, 0},   // Тёплый пыльный розовый  
      // {255, 248, 210, 0}    // Очень светлый кремово-жёлтый

      // // pixless pallete 1
      {13, 43, 69, 0},     // Очень тёмный индиго  
      {32, 60, 86, 0},     // Глубокий синий  
      {84, 78, 104, 0},    // Темный фиолетово-синий  
      {141, 105, 122, 0},  // Пыльный лиловый  
      {208, 129, 89, 0},   // Тёплый коричнево-оранжевый  
      {255, 170, 94, 0},   // Оранжевый ретро  
      {255, 212, 163, 0},  // Светлый персиковый  
      {255, 236, 214, 0}   // Очень светлый кремовый  
    };
    for (int i = 0; i < 8; i++) {
      client.write(palette[i], 4);
    }
  } else {
    for (int i = 0; i < 256; i++) {
      client.write((uint8_t)i); // B
      client.write((uint8_t)i); // G
      client.write((uint8_t)i); // R
      client.write((uint8_t)0); // Reserved
    }
  }

  // --- Big pixel buffer allocation and processing ---
  ensureBigPixelBuf(width * height);
  makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);

  // --- Crop if needed ---
  uint8_t* bmpBuf = g_bigPixelBuf;
  if (crop3_2) {
    static uint8_t croppedBuf[320 * 213];
    crop_center_3_2(g_bigPixelBuf, croppedBuf, width, height, cropHeight);
    bmpBuf = croppedBuf;
  }

  // --- Image data (bottom to top) ---
  uint8_t rowBuf[rowSize];
  for (int y = cropHeight - 1; y >= 0; y--) {
    size_t offset = y * width;
    if (use8ColorPalette) {
      int x = 0;
      for (; x + 1 < width; x += 2) {
        uint8_t v0 = bmpBuf[offset + x];
        uint8_t v1 = bmpBuf[offset + x + 1];
        uint8_t idx0, idx1;
        if (v0 < 32) idx0 = 0; else if (v0 < 64) idx0 = 1; else if (v0 < 96) idx0 = 2; else if (v0 < 128) idx0 = 3; else if (v0 < 160) idx0 = 4; else if (v0 < 192) idx0 = 5; else if (v0 < 224) idx0 = 6; else idx0 = 7;
        if (v1 < 32) idx1 = 0; else if (v1 < 64) idx1 = 1; else if (v1 < 96) idx1 = 2; else if (v1 < 128) idx1 = 3; else if (v1 < 160) idx1 = 4; else if (v1 < 192) idx1 = 5; else if (v1 < 224) idx1 = 6; else idx1 = 7;
        rowBuf[x] = idx0;
        rowBuf[x+1] = idx1;
      }
      if (x < width) {
        uint8_t v = bmpBuf[offset + x];
        uint8_t idx;
        if (v < 32) idx = 0; else if (v < 64) idx = 1; else if (v < 96) idx = 2; else if (v < 128) idx = 3; else if (v < 160) idx = 4; else if (v < 192) idx = 5; else if (v < 224) idx = 6; else idx = 7;
        rowBuf[x] = idx;
      }
    } else {
      memcpy(rowBuf, bmpBuf + offset, width);
    }
    for (int pad = width; pad < rowSize; pad++) rowBuf[pad] = 0;
    client.write(rowBuf, rowSize);
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
