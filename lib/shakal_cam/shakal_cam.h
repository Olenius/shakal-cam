#ifndef SHAKAL_CAM_H
#define SHAKAL_CAM_H

#include "esp_camera.h"
#include <WiFi.h>

void sendNewBMP(camera_fb_t *fb, WiFiClient &client);

#endif // SHAKAL_CAM_H
