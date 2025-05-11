#include "shakal_cam.h"
#include "png_encoder.h"
#include "lodepng.h"
#include <vector>

static uint8_t* g_bigPixelBuf = nullptr;
static size_t g_bigPixelBufSize = 0;

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
    const uint8_t palette[8*4] = {
        13, 43, 69, 255,
        32, 60, 86, 255,
        84, 78, 104, 255,
        141, 105, 122, 255,
        208, 129, 89, 255,
        255, 170, 94, 255,
        255, 212, 163, 255,
        255, 236, 214, 255
    };
    LodePNGState state;
    lodepng_state_init(&state);
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 8;
    lodepng_palette_clear(&state.info_raw);
    lodepng_palette_clear(&state.info_png.color);
    for (int i = 0; i < 8; ++i) {
        lodepng_palette_add(&state.info_raw, palette[i*4+0], palette[i*4+1], palette[i*4+2], palette[i*4+3]);
        lodepng_palette_add(&state.info_png.color, palette[i*4+0], palette[i*4+1], palette[i*4+2], palette[i*4+3]);
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
