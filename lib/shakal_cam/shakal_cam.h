#ifndef SHAKAL_CAM_H
#define SHAKAL_CAM_H

#include "esp_camera.h"
#include <WiFi.h>

// Объявление глобальной палитры
extern uint8_t g_colorPalette[8*4];

void sendNewBMP(camera_fb_t *fb, WiFiClient &client, bool use8ColorPalette);
void sendNewPNG(camera_fb_t *fb, WiFiClient &client);
void sendNewPNGWithPalette(camera_fb_t *fb, WiFiClient &client);
void generatePNGWithPaletteToRam(camera_fb_t *fb, uint8_t** pngData, size_t* pngSize);
void makeBigPixels8x8(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst);
void ensureBigPixelBuf(size_t size);
void crop_center_3_2(const uint8_t* src, uint8_t* dst, int width, int height, int cropHeight);

// Функции для работы с настройками палитры
bool loadPaletteFromSPIFFS();
bool savePaletteToSPIFFS();
void resetPaletteToDefault();
String getPaletteJSON();
bool updatePaletteFromJSON(const String& json);

#endif // SHAKAL_CAM_H
