#include "shakal_cam.h"
#include "png_encoder.h"
#include "lodepng.h"
#include <vector>
#include "FS.h"
#include "SPIFFS.h"

static uint8_t* g_bigPixelBuf = nullptr;
static size_t g_bigPixelBufSize = 0;

// Глобальная палитра с дефолтными значениями
uint8_t g_colorPalette[8*4] = {
    13, 43, 69, 255,    // Темно-синий
    32, 60, 86, 255,    // Синий
    84, 78, 104, 255,   // Фиолетовый
    141, 105, 122, 255, // Лиловый
    208, 129, 89, 255,  // Оранжевый
    255, 170, 94, 255,  // Светло-оранжевый
    255, 212, 163, 255, // Бежевый
    255, 236, 214, 255  // Светло-бежевый
};

// Функции для работы с настройками палитры
bool loadPaletteFromSPIFFS() {
    if (!SPIFFS.exists("/settings")) {
        SPIFFS.mkdir("/settings");
    }
    
    if (SPIFFS.exists("/settings/palette.bin")) {
        File file = SPIFFS.open("/settings/palette.bin", FILE_READ);
        if (file) {
            size_t read = file.read(g_colorPalette, sizeof(g_colorPalette));
            file.close();
            return (read == sizeof(g_colorPalette));
        }
    }
    return false;
}

bool savePaletteToSPIFFS() {
    if (!SPIFFS.exists("/settings")) {
        SPIFFS.mkdir("/settings");
    }
    
    File file = SPIFFS.open("/settings/palette.bin", FILE_WRITE);
    if (file) {
        size_t written = file.write(g_colorPalette, sizeof(g_colorPalette));
        file.close();
        return (written == sizeof(g_colorPalette));
    }
    return false;
}

void resetPaletteToDefault() {
    // Дефолтная палитра
    const uint8_t defaultPalette[8*4] = {
        13, 43, 69, 255,
        32, 60, 86, 255,
        84, 78, 104, 255,
        141, 105, 122, 255,
        208, 129, 89, 255,
        255, 170, 94, 255,
        255, 212, 163, 255,
        255, 236, 214, 255
    };
    
    memcpy(g_colorPalette, defaultPalette, sizeof(g_colorPalette));
    savePaletteToSPIFFS();
}

String getPaletteJSON() {
    String json = "{\"palette\":[";
    
    for (int i = 0; i < 8; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"r\":" + String(g_colorPalette[i*4+0]) + ",";
        json += "\"g\":" + String(g_colorPalette[i*4+1]) + ",";
        json += "\"b\":" + String(g_colorPalette[i*4+2]) + ",";
        json += "\"a\":" + String(g_colorPalette[i*4+3]);
        json += "}";
    }
    
    json += "]}";
    return json;
}

bool updatePaletteFromJSON(const String& jsonStr) {
    // Простой парсер JSON
    if (jsonStr.indexOf("\"palette\"") < 0) {
        return false;
    }
    
    int colorIndex = 0;
    int startPos = jsonStr.indexOf('[');
    int endPos = jsonStr.indexOf(']', startPos);
    
    if (startPos < 0 || endPos < 0) {
        return false;
    }
    
    String paletteArray = jsonStr.substring(startPos + 1, endPos);
    
    // Разбираем каждый цвет из массива
    int pos = 0;
    while (pos < paletteArray.length() && colorIndex < 8) {
        int colorObjStart = paletteArray.indexOf('{', pos);
        if (colorObjStart < 0) break;
        
        int colorObjEnd = paletteArray.indexOf('}', colorObjStart);
        if (colorObjEnd < 0) break;
        
        String colorObj = paletteArray.substring(colorObjStart, colorObjEnd + 1);
        
        // Извлекаем компоненты цвета
        int r = -1, g = -1, b = -1, a = 255;
        
        int rPos = colorObj.indexOf("\"r\"");
        if (rPos >= 0) {
            int valueStart = colorObj.indexOf(':', rPos) + 1;
            int valueEnd = colorObj.indexOf(',', valueStart);
            if (valueEnd < 0) valueEnd = colorObj.indexOf('}', valueStart);
            r = colorObj.substring(valueStart, valueEnd).toInt();
        }
        
        int gPos = colorObj.indexOf("\"g\"");
        if (gPos >= 0) {
            int valueStart = colorObj.indexOf(':', gPos) + 1;
            int valueEnd = colorObj.indexOf(',', valueStart);
            if (valueEnd < 0) valueEnd = colorObj.indexOf('}', valueStart);
            g = colorObj.substring(valueStart, valueEnd).toInt();
        }
        
        int bPos = colorObj.indexOf("\"b\"");
        if (bPos >= 0) {
            int valueStart = colorObj.indexOf(':', bPos) + 1;
            int valueEnd = colorObj.indexOf(',', valueStart);
            if (valueEnd < 0) valueEnd = colorObj.indexOf('}', valueStart);
            b = colorObj.substring(valueStart, valueEnd).toInt();
        }
        
        int aPos = colorObj.indexOf("\"a\"");
        if (aPos >= 0) {
            int valueStart = colorObj.indexOf(':', aPos) + 1;
            int valueEnd = colorObj.indexOf(',', valueStart);
            if (valueEnd < 0) valueEnd = colorObj.indexOf('}', valueStart);
            a = colorObj.substring(valueStart, valueEnd).toInt();
        }
        
        // Если компоненты извлечены успешно, обновляем палитру
        if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && a >= 0 && a <= 255) {
            g_colorPalette[colorIndex*4+0] = r;
            g_colorPalette[colorIndex*4+1] = g;
            g_colorPalette[colorIndex*4+2] = b;
            g_colorPalette[colorIndex*4+3] = a;
            colorIndex++;
        }
        
        pos = colorObjEnd + 1;
    }
    
    return (colorIndex == 8); // Все 8 цветов должны быть успешно обновлены
}

void ensureBigPixelBuf(size_t size) {
    if (!g_bigPixelBuf || g_bigPixelBufSize < size) {
        if (g_bigPixelBuf) free(g_bigPixelBuf);
        g_bigPixelBuf = (uint8_t*)malloc(size);
        g_bigPixelBufSize = size;
    }
}

void crop_center_3_2(const uint8_t* src, uint8_t* dst, int width, int height, int cropHeight) {
    int y0 = (height - cropHeight) / 2;
    memcpy(dst, src + y0 * width, width * cropHeight);
}

void makeBigPixels8x8(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst) {
    const int blockSize = 4;
    for (int by = 0; by < srcHeight; by += blockSize) {
        int blockH = (by + blockSize <= srcHeight) ? blockSize : (srcHeight - by);
        for (int bx = 0; bx < srcWidth; bx += blockSize) {
            int blockW = (bx + blockSize <= srcWidth) ? blockSize : (srcWidth - bx);
            int sum = 0;
            for (int y = 0; y < blockH; ++y) {
                const uint8_t* rowPtr = src + (by + y) * srcWidth + bx;
                for (int x = 0; x < blockW; ++x) {
                    sum += rowPtr[x];
                }
            }
            uint8_t avg = sum / (blockW * blockH);
            for (int y = 0; y < blockH; ++y) {
                uint8_t* dstRow = dst + (by + y) * srcWidth + bx;
                for (int x = 0; x < blockW; ++x) {
                    dstRow[x] = avg;
                }
            }
        }
    }
}

void sendNewPNGWithPalette(camera_fb_t *fb, WiFiClient &client) {
    int width = fb->width;
    int height = fb->height;
    bool crop3_2 = (width == 320 && height == 240);
    int cropHeight = crop3_2 ? 213 : height;
    ensureBigPixelBuf(width * height);
    makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);
    const uint8_t* srcBuf = g_bigPixelBuf;
    int pngWidth = width;
    int pngHeight = height;
    uint8_t* croppedBuf = nullptr;
    if (crop3_2) {
        croppedBuf = (uint8_t*)malloc(width * cropHeight);
        if (!croppedBuf) return;
        crop_center_3_2(g_bigPixelBuf, croppedBuf, width, height, cropHeight);
        srcBuf = croppedBuf;
        pngWidth = width;
        pngHeight = cropHeight;
    }
    std::vector<uint8_t> idxBuf(pngWidth * pngHeight);
    for (int y = 0; y < pngHeight; ++y) {
        for (int x = 0; x < pngWidth; ++x) {
            uint8_t v = srcBuf[y * pngWidth + x];
            uint8_t idx;
            if (v < 32) idx = 0;
            else if (v < 64) idx = 1;
            else if (v < 96) idx = 2;
            else if (v < 128) idx = 3;
            else if (v < 160) idx = 4;
            else if (v < 192) idx = 5;
            else if (v < 224) idx = 6;
            else idx = 7;
            idxBuf[y * pngWidth + x] = idx;
        }
    }
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;
    lodepng_palette_clear(&state.info_raw);
    lodepng_palette_clear(&state.info_png.color);
    for (int i = 0; i < 8; ++i) {
        lodepng_palette_add(&state.info_raw, g_colorPalette[i*4+0], g_colorPalette[i*4+1], g_colorPalette[i*4+2], g_colorPalette[i*4+3]);
        lodepng_palette_add(&state.info_png.color, g_colorPalette[i*4+0], g_colorPalette[i*4+1], g_colorPalette[i*4+2], g_colorPalette[i*4+3]);
    }
    unsigned char* out = nullptr;
    size_t outsize = 0;
    unsigned error = lodepng_encode(&out, &outsize, idxBuf.data(), pngWidth, pngHeight, &state);
    lodepng_state_cleanup(&state);
    if (error) {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println();
        client.print("PNG encode error: ");
        client.print(error);
        client.print(" ");
        client.print(lodepng_error_text(error));
        if (out) free(out);
        if (croppedBuf) free(croppedBuf);
        return;
    }
    client.write(out, outsize);
    if (out) free(out);
    if (croppedBuf) free(croppedBuf);
}

void generatePNGWithPaletteToRam(camera_fb_t *fb, uint8_t** pngData, size_t* pngSize) {
    int width = fb->width;
    int height = fb->height;
    bool crop3_2 = (width == 320 && height == 240);
    int cropHeight = crop3_2 ? 213 : height;
    ensureBigPixelBuf(width * height);
    makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);
    const uint8_t* srcBuf = g_bigPixelBuf;
    int pngWidth = width;
    int pngHeight = height;
    uint8_t* croppedBuf = nullptr;
    if (crop3_2) {
        croppedBuf = (uint8_t*)malloc(width * cropHeight);
        if (!croppedBuf) {
            *pngData = nullptr;
            *pngSize = 0;
            return;
        }
        crop_center_3_2(g_bigPixelBuf, croppedBuf, width, height, cropHeight);
        srcBuf = croppedBuf;
        pngWidth = width;
        pngHeight = cropHeight;
    }
    std::vector<uint8_t> idxBuf(pngWidth * pngHeight);
    for (int y = 0; y < pngHeight; ++y) {
        for (int x = 0; x < pngWidth; ++x) {
            uint8_t v = srcBuf[y * pngWidth + x];
            uint8_t idx;
            if (v < 32) idx = 0;
            else if (v < 64) idx = 1;
            else if (v < 96) idx = 2;
            else if (v < 128) idx = 3;
            else if (v < 160) idx = 4;
            else if (v < 192) idx = 5;
            else if (v < 224) idx = 6;
            else idx = 7;
            idxBuf[y * pngWidth + x] = idx;
        }
    }
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;
    lodepng_palette_clear(&state.info_raw);
    lodepng_palette_clear(&state.info_png.color);
    for (int i = 0; i < 8; ++i) {
        lodepng_palette_add(&state.info_raw, g_colorPalette[i*4+0], g_colorPalette[i*4+1], g_colorPalette[i*4+2], g_colorPalette[i*4+3]);
        lodepng_palette_add(&state.info_png.color, g_colorPalette[i*4+0], g_colorPalette[i*4+1], g_colorPalette[i*4+2], g_colorPalette[i*4+3]);
    }
    unsigned char* out = nullptr;
    size_t outsize = 0;
    unsigned error = lodepng_encode(&out, &outsize, idxBuf.data(), pngWidth, pngHeight, &state);
    lodepng_state_cleanup(&state);
    if (error) {
        Serial.printf("PNG encode error: %u %s\n", error, lodepng_error_text(error));
        if (out) free(out);
        if (croppedBuf) free(croppedBuf);
        *pngData = nullptr;
        *pngSize = 0;
        return;
    }
    
    *pngData = out;
    *pngSize = outsize;
    
    if (croppedBuf) free(croppedBuf);
}
