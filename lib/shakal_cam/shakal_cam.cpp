#include "shakal_cam.h"
#include "png_encoder.h"
#include "lodepng.h"
#include <vector>
#include "FS.h"
#include "SPIFFS.h"
#include "img_converters.h"

// Global buffer for image processing
static uint8_t* g_bigPixelBuf = nullptr;
static size_t g_bigPixelBufSize = 0;

// Global color palette with default values
uint8_t g_colorPalette[8*4] = {
    13, 43, 69, 255,    // Dark blue
    32, 60, 86, 255,    // Blue
    84, 78, 104, 255,   // Purple
    141, 105, 122, 255, // Lilac
    208, 129, 89, 255,  // Orange
    255, 170, 94, 255,  // Light orange
    255, 212, 163, 255, // Beige
    255, 236, 214, 255  // Light beige
};

// Global block size setting with default value
int g_blockSize = 2;

// Global flag to control output format (true = JPEG, false = PNG)
bool g_useJpegOutput = false;  // Default to PNG for compatibility

/**
 * Loads the color palette from SPIFFS filesystem
 * 
 * @return true if palette successfully loaded, false otherwise
 */
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

/**
 * Saves the current color palette to SPIFFS filesystem
 * 
 * @return true if palette successfully saved, false otherwise
 */
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

/**
 * Resets the color palette to default values
 */
void resetPaletteToDefault() {
    // Default palette
    const uint8_t defaultPalette[8*4] = {
        13, 43, 69, 255,    // Dark blue
        32, 60, 86, 255,    // Blue
        84, 78, 104, 255,   // Purple
        141, 105, 122, 255, // Lilac
        208, 129, 89, 255,  // Orange
        255, 170, 94, 255,  // Light orange
        255, 212, 163, 255, // Beige
        255, 236, 214, 255  // Light beige
    };
    
    memcpy(g_colorPalette, defaultPalette, sizeof(g_colorPalette));
    savePaletteToSPIFFS();
}

/**
 * Generates a JSON string representation of the current color palette
 * 
 * @return String containing the palette in JSON format
 */
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

/**
 * Updates the color palette from a JSON string
 * 
 * @param jsonStr JSON string containing palette data
 * @return true if palette successfully updated, false otherwise
 */
bool updatePaletteFromJSON(const String& jsonStr) {
    // Simple JSON parser for palette data
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
    
    // Parse each color from the array
    int pos = 0;
    while (pos < paletteArray.length() && colorIndex < 8) {
        int colorObjStart = paletteArray.indexOf('{', pos);
        if (colorObjStart < 0) break;
        
        int colorObjEnd = paletteArray.indexOf('}', colorObjStart);
        if (colorObjEnd < 0) break;
        
        String colorObj = paletteArray.substring(colorObjStart, colorObjEnd + 1);
        
        // Extract color components
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
        
        // Update palette if components were successfully extracted
        if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && a >= 0 && a <= 255) {
            g_colorPalette[colorIndex*4+0] = r;
            g_colorPalette[colorIndex*4+1] = g;
            g_colorPalette[colorIndex*4+2] = b;
            g_colorPalette[colorIndex*4+3] = a;
            colorIndex++;
        }
        
        pos = colorObjEnd + 1;
    }
    
    return (colorIndex == 8); // All 8 colors must be successfully updated
}

/**
 * Loads the block size setting from SPIFFS filesystem
 * 
 * @return true if block size successfully loaded, false otherwise
 */
bool loadBlockSizeFromSPIFFS() {
    if (!SPIFFS.exists("/settings")) {
        SPIFFS.mkdir("/settings");
    }
    
    if (SPIFFS.exists("/settings/blocksize.bin")) {
        File file = SPIFFS.open("/settings/blocksize.bin", FILE_READ);
        if (file) {
            size_t read = file.read((uint8_t*)&g_blockSize, sizeof(g_blockSize));
            file.close();
            if (read == sizeof(g_blockSize) && g_blockSize >= 1 && g_blockSize <= 16) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Saves the current block size setting to SPIFFS filesystem
 * 
 * @return true if block size successfully saved, false otherwise
 */
bool saveBlockSizeToSPIFFS() {
    if (!SPIFFS.exists("/settings")) {
        SPIFFS.mkdir("/settings");
    }
    
    File file = SPIFFS.open("/settings/blocksize.bin", FILE_WRITE);
    if (file) {
        size_t written = file.write((uint8_t*)&g_blockSize, sizeof(g_blockSize));
        file.close();
        return (written == sizeof(g_blockSize));
    }
    return false;
}

/**
 * Resets the block size to default value
 */
void resetBlockSizeToDefault() {
    g_blockSize = 2;
    saveBlockSizeToSPIFFS();
}

/**
 * Generates a JSON string representation of the current block size
 * 
 * @return String containing the block size in JSON format
 */
String getBlockSizeJSON() {
    return "{\"blockSize\":" + String(g_blockSize) + "}";
}

/**
 * Updates the block size from a JSON string
 * 
 * @param jsonStr JSON string containing block size data
 * @return true if block size successfully updated, false otherwise
 */
bool updateBlockSizeFromJSON(const String& jsonStr) {
    int blockSizePos = jsonStr.indexOf("\"blockSize\"");
    if (blockSizePos < 0) {
        return false;
    }
    
    int valueStart = jsonStr.indexOf(':', blockSizePos) + 1;
    int valueEnd = jsonStr.indexOf(',', valueStart);
    if (valueEnd < 0) valueEnd = jsonStr.indexOf('}', valueStart);
    
    if (valueStart > 0 && valueEnd > valueStart) {
        int newBlockSize = jsonStr.substring(valueStart, valueEnd).toInt();
        if (newBlockSize >= 1 && newBlockSize <= 16) {
            g_blockSize = newBlockSize;
            return true;
        }
    }
    
    return false;
}

/**
 * Ensures the global pixel buffer is large enough for image processing
 * 
 * @param size Required buffer size in bytes
 */
void ensureBigPixelBuf(size_t size) {
    if (!g_bigPixelBuf || g_bigPixelBufSize < size) {
        if (g_bigPixelBuf) free(g_bigPixelBuf);
        g_bigPixelBuf = (uint8_t*)malloc(size);
        g_bigPixelBufSize = size;
    }
}

/**
 * Crops the center portion of an image for 3:2 aspect ratio
 * 
 * @param src Source image data
 * @param dst Destination buffer for cropped image
 * @param width Image width
 * @param height Image height
 * @param cropHeight Desired height after cropping
 */
void crop_center_3_2(const uint8_t* src, uint8_t* dst, int width, int height, int cropHeight) {
    int y0 = (height - cropHeight) / 2;
    memcpy(dst, src + y0 * width, width * cropHeight);
}

/**
 * Creates a blocky/pixelated effect by averaging 4x4 pixel blocks
 * 
 * @param src Source image data
 * @param srcWidth Source image width
 * @param srcHeight Source image height
 * @param dst Destination buffer for processed image
 */
void makeBigPixels8x8(const uint8_t* src, int srcWidth, int srcHeight, uint8_t* dst) {
    unsigned long pixelationAlgorithmStart = millis();
    
    const int blockSize = g_blockSize;
    int totalBlocks = 0;
    
    for (int by = 0; by < srcHeight; by += blockSize) {
        int blockH = (by + blockSize <= srcHeight) ? blockSize : (srcHeight - by);
        for (int bx = 0; bx < srcWidth; bx += blockSize) {
            int blockW = (bx + blockSize <= srcWidth) ? blockSize : (srcWidth - bx);
            int sum = 0;
            
            // Calculate average of all pixels in the block
            for (int y = 0; y < blockH; ++y) {
                const uint8_t* rowPtr = src + (by + y) * srcWidth + bx;
                for (int x = 0; x < blockW; ++x) {
                    sum += rowPtr[x];
                }
            }
            uint8_t avg = sum / (blockW * blockH);
            
            // Set all pixels in the block to the average value
            for (int y = 0; y < blockH; ++y) {
                uint8_t* dstRow = dst + (by + y) * srcWidth + bx;
                for (int x = 0; x < blockW; ++x) {
                    dstRow[x] = avg;
                }
            }
            totalBlocks++;
        }
    }
    
    unsigned long pixelationAlgorithmEnd = millis();
    Serial.printf("    Core pixelation algorithm took: %lu ms (processed %d blocks of size %dx%d)\n", 
                  pixelationAlgorithmEnd - pixelationAlgorithmStart, totalBlocks, blockSize, blockSize);
}

/**
 * Sends a processed image as PNG with the current color palette directly to client
 * 
 * @param fb Camera frame buffer
 * @param client WiFi client to send PNG to
 */
void sendNewPNGWithPalette(camera_fb_t *fb, WiFiClient &client) {
    unsigned long totalStartTime = millis();
    
    int width = fb->width;
    int height = fb->height;
    bool crop3_2 = (width == 320 && height == 240);
    int cropHeight = crop3_2 ? 213 : height;
    
    // Process the image to create the pixelated effect
    unsigned long pixelationStartTime = millis();
    ensureBigPixelBuf(width * height);
    makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);
    unsigned long pixelationEndTime = millis();
    Serial.printf("  Pixelation step took: %lu ms\n", pixelationEndTime - pixelationStartTime);
    
    const uint8_t* srcBuf = g_bigPixelBuf;
    int pngWidth = width;
    int pngHeight = height;
    uint8_t* croppedBuf = nullptr;
    
    // Crop to 3:2 aspect ratio if needed
    unsigned long cropStartTime = millis();
    if (crop3_2) {
        croppedBuf = (uint8_t*)malloc(width * cropHeight);
        if (!croppedBuf) return;
        crop_center_3_2(g_bigPixelBuf, croppedBuf, width, height, cropHeight);
        srcBuf = croppedBuf;
        pngWidth = width;
        pngHeight = cropHeight;
    }
    unsigned long cropEndTime = millis();
    if (crop3_2) {
        Serial.printf("  Cropping step took: %lu ms\n", cropEndTime - cropStartTime);
    }
    
    // Create indexed color buffer using our 8-color palette
    unsigned long paletteStartTime = millis();
    std::vector<uint8_t> idxBuf(pngWidth * pngHeight);
    for (int y = 0; y < pngHeight; ++y) {
        for (int x = 0; x < pngWidth; ++x) {
            uint8_t v = srcBuf[y * pngWidth + x];
            uint8_t idx;
            // Map grayscale value to palette index
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
    unsigned long paletteEndTime = millis();
    Serial.printf("  Palette mapping step took: %lu ms\n", paletteEndTime - paletteStartTime);
    
    // Configure PNG encoder to use our palette
    unsigned long pngSetupStartTime = millis();
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;
    lodepng_palette_clear(&state.info_raw);
    lodepng_palette_clear(&state.info_png.color);
    
    // Add palette colors to PNG
    for (int i = 0; i < 8; ++i) {
        lodepng_palette_add(&state.info_raw, 
                            g_colorPalette[i*4+0], 
                            g_colorPalette[i*4+1], 
                            g_colorPalette[i*4+2], 
                            g_colorPalette[i*4+3]);
        lodepng_palette_add(&state.info_png.color, 
                            g_colorPalette[i*4+0], 
                            g_colorPalette[i*4+1], 
                            g_colorPalette[i*4+2], 
                            g_colorPalette[i*4+3]);
    }
    unsigned long pngSetupEndTime = millis();
    Serial.printf("  PNG setup step took: %lu ms\n", pngSetupEndTime - pngSetupStartTime);
    
    // Encode PNG data
    unsigned long pngEncodeStartTime = millis();
    unsigned char* out = nullptr;
    size_t outsize = 0;
    unsigned error = lodepng_encode(&out, &outsize, idxBuf.data(), pngWidth, pngHeight, &state);
    lodepng_state_cleanup(&state);
    unsigned long pngEncodeEndTime = millis();
    Serial.printf("  PNG encoding step took: %lu ms\n", pngEncodeEndTime - pngEncodeStartTime);
    
    // Handle encoding errors
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
    
    // Send the PNG data to client
    unsigned long sendStartTime = millis();
    client.write(out, outsize);
    unsigned long sendEndTime = millis();
    Serial.printf("  Data transmission step took: %lu ms\n", sendEndTime - sendStartTime);
    
    // Clean up
    if (out) free(out);
    if (croppedBuf) free(croppedBuf);
    
    unsigned long totalEndTime = millis();
    Serial.printf("  Total sendNewPNGWithPalette time: %lu ms\n", totalEndTime - totalStartTime);
}

/**
 * Generates a PNG image with the current color palette and stores it in RAM
 * 
 * @param fb Camera frame buffer
 * @param pngData Pointer to store the generated PNG data pointer (caller must free)
 * @param pngSize Pointer to store the size of generated PNG
 */
void generatePNGWithPaletteToRam(camera_fb_t *fb, uint8_t** pngData, size_t* pngSize) {
    unsigned long totalStartTime = millis();
    
    int width = fb->width;
    int height = fb->height;
    bool crop3_2 = (width == 320 && height == 240);
    int cropHeight = crop3_2 ? 213 : height;
    
    // Process the image to create the pixelated effect
    unsigned long pixelationStartTime = millis();
    ensureBigPixelBuf(width * height);
    makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);
    unsigned long pixelationEndTime = millis();
    Serial.printf("  [RAM] Pixelation step took: %lu ms\n", pixelationEndTime - pixelationStartTime);
    
    const uint8_t* srcBuf = g_bigPixelBuf;
    int pngWidth = width;
    int pngHeight = height;
    uint8_t* croppedBuf = nullptr;
    
    // Crop to 3:2 aspect ratio if needed
    unsigned long cropStartTime = millis();
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
    unsigned long cropEndTime = millis();
    if (crop3_2) {
        Serial.printf("  [RAM] Cropping step took: %lu ms\n", cropEndTime - cropStartTime);
    }
    
    // Create indexed color buffer using our 8-color palette
    unsigned long paletteStartTime = millis();
    std::vector<uint8_t> idxBuf(pngWidth * pngHeight);
    for (int y = 0; y < pngHeight; ++y) {
        for (int x = 0; x < pngWidth; ++x) {
            uint8_t v = srcBuf[y * pngWidth + x];
            uint8_t idx;
            // Map grayscale value to palette index
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
    unsigned long paletteEndTime = millis();
    Serial.printf("  [RAM] Palette mapping step took: %lu ms\n", paletteEndTime - paletteStartTime);
    
    // Configure PNG encoder to use our palette
    unsigned long pngSetupStartTime = millis();
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;
    lodepng_palette_clear(&state.info_raw);
    lodepng_palette_clear(&state.info_png.color);
    
    // Add palette colors to PNG
    for (int i = 0; i < 8; ++i) {
        lodepng_palette_add(&state.info_raw, 
                            g_colorPalette[i*4+0], 
                            g_colorPalette[i*4+1], 
                            g_colorPalette[i*4+2], 
                            g_colorPalette[i*4+3]);
        lodepng_palette_add(&state.info_png.color, 
                            g_colorPalette[i*4+0], 
                            g_colorPalette[i*4+1], 
                            g_colorPalette[i*4+2], 
                            g_colorPalette[i*4+3]);
    }
    unsigned long pngSetupEndTime = millis();
    Serial.printf("  [RAM] PNG setup step took: %lu ms\n", pngSetupEndTime - pngSetupStartTime);
    
    // Encode PNG data
    unsigned long pngEncodeStartTime = millis();
    unsigned char* out = nullptr;
    size_t outsize = 0;
    unsigned error = lodepng_encode(&out, &outsize, idxBuf.data(), pngWidth, pngHeight, &state);
    lodepng_state_cleanup(&state);
    unsigned long pngEncodeEndTime = millis();
    Serial.printf("  [RAM] PNG encoding step took: %lu ms\n", pngEncodeEndTime - pngEncodeStartTime);
    
    // Handle encoding errors
    if (error) {
        Serial.printf("PNG encode error: %u %s\n", error, lodepng_error_text(error));
        if (out) free(out);
        if (croppedBuf) free(croppedBuf);
        *pngData = nullptr;
        *pngSize = 0;
        return;
    }
    
    // Set output parameters
    *pngData = out;
    *pngSize = outsize;
    
    // Clean up
    if (croppedBuf) free(croppedBuf);
    
    unsigned long totalEndTime = millis();
    Serial.printf("  [RAM] Total generatePNGWithPaletteToRam time: %lu ms\n", totalEndTime - totalStartTime);
}

/**
 * Sends a processed image as JPEG with pixelation directly to client
 * 
 * @param fb Camera frame buffer
 * @param client WiFi client to send JPEG to
 */
void sendNewJPEGWithPixelation(camera_fb_t *fb, WiFiClient &client) {
    unsigned long totalStartTime = millis();
    
    int width = fb->width;
    int height = fb->height;
    bool crop3_2 = (width == 320 && height == 240);
    int cropHeight = crop3_2 ? 213 : height;
    
    // Process the image to create the pixelated effect
    unsigned long pixelationStartTime = millis();
    ensureBigPixelBuf(width * height);
    makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);
    unsigned long pixelationEndTime = millis();
    Serial.printf("  [JPEG] Pixelation step took: %lu ms\n", pixelationEndTime - pixelationStartTime);
    
    const uint8_t* srcBuf = g_bigPixelBuf;
    int finalWidth = width;
    int finalHeight = height;
    uint8_t* croppedBuf = nullptr;
    
    // Crop to 3:2 aspect ratio if needed
    unsigned long cropStartTime = millis();
    if (crop3_2) {
        croppedBuf = (uint8_t*)malloc(width * cropHeight);
        if (!croppedBuf) return;
        crop_center_3_2(g_bigPixelBuf, croppedBuf, width, height, cropHeight);
        srcBuf = croppedBuf;
        finalWidth = width;
        finalHeight = cropHeight;
    }
    unsigned long cropEndTime = millis();
    if (crop3_2) {
        Serial.printf("  [JPEG] Cropping step took: %lu ms\n", cropEndTime - cropStartTime);
    }
    
    // Convert grayscale to JPEG
    unsigned long jpegEncodeStartTime = millis();
    uint8_t* jpegBuf = nullptr;
    size_t jpegLen = 0;
    
    // Use ESP32's built-in JPEG encoder for grayscale
    bool success = fmt2jpg((uint8_t*)srcBuf, finalWidth * finalHeight, finalWidth, finalHeight, PIXFORMAT_GRAYSCALE, 80, &jpegBuf, &jpegLen);
    unsigned long jpegEncodeEndTime = millis();
    Serial.printf("  [JPEG] JPEG encoding step took: %lu ms\n", jpegEncodeEndTime - jpegEncodeStartTime);
    
    if (!success || !jpegBuf) {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println();
        client.println("JPEG encoding failed");
        if (croppedBuf) free(croppedBuf);
        return;
    }
    
    // Send the JPEG data to client
    unsigned long sendStartTime = millis();
    client.write(jpegBuf, jpegLen);
    unsigned long sendEndTime = millis();
    Serial.printf("  [JPEG] Data transmission step took: %lu ms\n", sendEndTime - sendStartTime);
    
    // Clean up
    if (jpegBuf) free(jpegBuf);
    if (croppedBuf) free(croppedBuf);
    
    unsigned long totalEndTime = millis();
    Serial.printf("  [JPEG] Total sendNewJPEGWithPixelation time: %lu ms\n", totalEndTime - totalStartTime);
}

/**
 * Generates a JPEG image with pixelation and stores it in RAM
 * 
 * @param fb Camera frame buffer
 * @param jpegData Pointer to store the generated JPEG data pointer (caller must free)
 * @param jpegSize Pointer to store the size of generated JPEG
 */
void generateJPEGWithPixelationToRam(camera_fb_t *fb, uint8_t** jpegData, size_t* jpegSize) {
    unsigned long totalStartTime = millis();
    
    int width = fb->width;
    int height = fb->height;
    bool crop3_2 = (width == 320 && height == 240);
    int cropHeight = crop3_2 ? 213 : height;
    
    // Process the image to create the pixelated effect
    unsigned long pixelationStartTime = millis();
    ensureBigPixelBuf(width * height);
    makeBigPixels8x8(fb->buf, width, height, g_bigPixelBuf);
    unsigned long pixelationEndTime = millis();
    Serial.printf("  [JPEG-RAM] Pixelation step took: %lu ms\n", pixelationEndTime - pixelationStartTime);
    
    const uint8_t* srcBuf = g_bigPixelBuf;
    int finalWidth = width;
    int finalHeight = height;
    uint8_t* croppedBuf = nullptr;
    
    // Crop to 3:2 aspect ratio if needed
    unsigned long cropStartTime = millis();
    if (crop3_2) {
        croppedBuf = (uint8_t*)malloc(width * cropHeight);
        if (!croppedBuf) {
            *jpegData = nullptr;
            *jpegSize = 0;
            return;
        }
        crop_center_3_2(g_bigPixelBuf, croppedBuf, width, height, cropHeight);
        srcBuf = croppedBuf;
        finalWidth = width;
        finalHeight = cropHeight;
    }
    unsigned long cropEndTime = millis();
    if (crop3_2) {
        Serial.printf("  [JPEG-RAM] Cropping step took: %lu ms\n", cropEndTime - cropStartTime);
    }
    
    // Convert grayscale to JPEG
    unsigned long jpegEncodeStartTime = millis();
    uint8_t* jpegBuf = nullptr;
    size_t jpegLen = 0;
    
    // Use ESP32's built-in JPEG encoder for grayscale
    bool success = fmt2jpg((uint8_t*)srcBuf, finalWidth * finalHeight, finalWidth, finalHeight, PIXFORMAT_GRAYSCALE, 80, &jpegBuf, &jpegLen);
    unsigned long jpegEncodeEndTime = millis();
    Serial.printf("  [JPEG-RAM] JPEG encoding step took: %lu ms\n", jpegEncodeEndTime - jpegEncodeStartTime);
    
    if (!success || !jpegBuf) {
        Serial.println("JPEG encoding failed");
        if (croppedBuf) free(croppedBuf);
        *jpegData = nullptr;
        *jpegSize = 0;
        return;
    }
    
    // Set output parameters
    *jpegData = jpegBuf;
    *jpegSize = jpegLen;
    
    // Clean up
    if (croppedBuf) free(croppedBuf);
    
    unsigned long totalEndTime = millis();
    Serial.printf("  [JPEG-RAM] Total generateJPEGWithPixelationToRam time: %lu ms\n", totalEndTime - totalStartTime);
}
