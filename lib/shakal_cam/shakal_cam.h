#ifndef SHAKAL_CAM_H
#define SHAKAL_CAM_H

#include "esp_camera.h"
#include <WiFi.h>

void sendNewBMP(camera_fb_t *fb, WiFiClient &client, bool use8ColorPalette);
void makeBigPixels8x8(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst);

#endif // SHAKAL_CAM_H
