#include "png_encoder.h"

// --- Minimal CRC32 and Adler32 for PNG (no zlib needed) ---
static uint32_t crc32_table[256];
static void crc32_init() {
    uint32_t poly = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = c & 1 ? poly ^ (c >> 1) : c >> 1;
        crc32_table[i] = c;
    }
}
static uint32_t crc32(uint32_t crc, const uint8_t* buf, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; i++)
        crc = crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}
static uint32_t adler32(uint32_t adler, const uint8_t* buf, size_t len) {
    uint32_t a = adler & 0xFFFF, b = (adler >> 16) & 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        a = (a + buf[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

// Helper: Write 4 bytes big-endian
template<typename T>
static void writeBE(T& client, uint32_t v) {
    client.write((uint8_t)(v >> 24));
    client.write((uint8_t)(v >> 16));
    client.write((uint8_t)(v >> 8));
    client.write((uint8_t)(v));
}

// Helper: Write PNG chunk
template<typename T>
static void writeChunk(T& client, const char* type, const uint8_t* data, size_t len) {
    writeBE(client, (uint32_t)len);
    client.write((const uint8_t*)type, 4);
    if (len) client.write(data, len);
    uint32_t crc = crc32(0, (const uint8_t*)type, 4);
    if (len) crc = crc32(crc, data, len);
    writeBE(client, crc);
}

// Minimal PNG encoder for 8-bit grayscale, no compression (uncompressed DEFLATE block)
bool encodePNGGray8ToClient(const uint8_t* image, int width, int height, WiFiClient& client) {
    static bool crc_inited = false;
    if (!crc_inited) { crc32_init(); crc_inited = true; }
    // PNG signature
    const uint8_t sig[8] = {137,80,78,71,13,10,26,10};
    client.write(sig, 8);
    // IHDR
    uint8_t ihdr[13] = {
        (uint8_t)(width>>24),(uint8_t)(width>>16),(uint8_t)(width>>8),(uint8_t)width,
        (uint8_t)(height>>24),(uint8_t)(height>>16),(uint8_t)(height>>8),(uint8_t)height,
        8, // bit depth
        0, // color type: grayscale
        0, // compression
        0, // filter
        0  // interlace
    };
    writeChunk(client, "IHDR", ihdr, 13);
    // IDAT (uncompressed DEFLATE block)
    size_t rowLen = 1 + width; // filter byte + data
    size_t rawLen = rowLen * height;
    size_t zlen = 2 + rawLen + (rawLen/65535+1)*5 + 4; // zlib header+adler32+block headers
    uint8_t* zbuf = (uint8_t*)malloc(zlen);
    if (!zbuf) return false;
    zbuf[0] = 0x78; zbuf[1] = 0x01; // zlib header (no compression)
    size_t pos = 2;
    uint32_t adler = 1;
    for (int y = 0; y < height; ++y) {
        size_t off = y * rowLen;
        zbuf[pos++] = 0; // filter type 0
        memcpy(zbuf+pos, image+y*width, width);
        adler = adler32(adler, image+y*width, width);
        pos += width;
    }
    // Write uncompressed DEFLATE block header
    size_t datalen = rawLen;
    size_t dpos = 2;
    size_t srcpos = 2;
    while (datalen > 0) {
        size_t blk = datalen > 65535 ? 65535 : datalen;
        zbuf[dpos++] = (datalen <= 65535) ? 1 : 0; // BFINAL
        zbuf[dpos++] = blk & 0xFF;
        zbuf[dpos++] = (blk >> 8) & 0xFF;
        zbuf[dpos++] = (~blk) & 0xFF;
        zbuf[dpos++] = ((~blk) >> 8) & 0xFF;
        memcpy(zbuf + dpos, zbuf + srcpos, blk); // исправлено: копируем из подготовленных строк
        dpos += blk;
        srcpos += blk;
        datalen -= blk;
    }
    // Adler32
    zbuf[dpos++] = (adler >> 24) & 0xFF;
    zbuf[dpos++] = (adler >> 16) & 0xFF;
    zbuf[dpos++] = (adler >> 8) & 0xFF;
    zbuf[dpos++] = adler & 0xFF;
    writeChunk(client, "IDAT", zbuf, dpos);
    free(zbuf);
    // IEND
    writeChunk(client, "IEND", nullptr, 0);
    return true;
}

// Minimal PNG encoder for 8-bit palette (indexed color, 8 colors), no compression
bool encodePNGPalette8ToClient(const uint8_t* image, int width, int height, const uint8_t* palette, int paletteSize, WiFiClient& client) {
    static bool crc_inited = false;
    if (!crc_inited) { crc32_init(); crc_inited = true; }
    // PNG signature
    const uint8_t sig[8] = {137,80,78,71,13,10,26,10};
    client.write(sig, 8);
    // IHDR
    uint8_t ihdr[13] = {
        (uint8_t)(width>>24),(uint8_t)(width>>16),(uint8_t)(width>>8),(uint8_t)width,
        (uint8_t)(height>>24),(uint8_t)(height>>16),(uint8_t)(height>>8),(uint8_t)height,
        8, // bit depth
        3, // color type: palette
        0, // compression
        0, // filter
        0  // interlace
    };
    writeChunk(client, "IHDR", ihdr, 13);
    // PLTE chunk (RGB only)
    uint8_t plte[256*3] = {0};
    for (int i = 0; i < paletteSize; ++i) {
        plte[i*3+0] = palette[i*4+0];
        plte[i*3+1] = palette[i*4+1];
        plte[i*3+2] = palette[i*4+2];
    }
    writeChunk(client, "PLTE", plte, paletteSize*3);
    // IDAT (uncompressed DEFLATE block)
    size_t rowLen = 1 + width; // filter byte + data
    size_t rawLen = rowLen * height;
    size_t zlen = 2 + rawLen + (rawLen/65535+1)*5 + 4;
    uint8_t* zbuf = (uint8_t*)malloc(zlen);
    if (!zbuf) return false;
    zbuf[0] = 0x78; zbuf[1] = 0x01; // zlib header (no compression)
    size_t pos = 2;
    uint32_t adler = 1;
    for (int y = 0; y < height; ++y) {
        size_t off = y * rowLen;
        zbuf[pos++] = 0; // filter type 0
        memcpy(zbuf+pos, image+y*width, width);
        adler = adler32(adler, image+y*width, width);
        pos += width;
    }
    // Write uncompressed DEFLATE block header
    size_t datalen = rawLen;
    size_t dpos = 2;
    size_t srcpos = 2;
    while (datalen > 0) {
        size_t blk = datalen > 65535 ? 65535 : datalen;
        zbuf[dpos++] = (datalen <= 65535) ? 1 : 0; // BFINAL
        zbuf[dpos++] = blk & 0xFF;
        zbuf[dpos++] = (blk >> 8) & 0xFF;
        zbuf[dpos++] = (~blk) & 0xFF;
        zbuf[dpos++] = ((~blk) >> 8) & 0xFF;
        memcpy(zbuf + dpos, zbuf + srcpos, blk);
        dpos += blk;
        srcpos += blk;
        datalen -= blk;
    }
    // Adler32
    zbuf[dpos++] = (adler >> 24) & 0xFF;
    zbuf[dpos++] = (adler >> 16) & 0xFF;
    zbuf[dpos++] = (adler >> 8) & 0xFF;
    zbuf[dpos++] = adler & 0xFF;
    writeChunk(client, "IDAT", zbuf, dpos);
    free(zbuf);
    // IEND
    writeChunk(client, "IEND", nullptr, 0);
    return true;
}

size_t encodePNGGray8ToBuffer(const uint8_t* image, int width, int height, uint8_t* outBuf, size_t outBufSize) {
    // Not implemented for now (for simplicity, use client version)
    return 0;
}
