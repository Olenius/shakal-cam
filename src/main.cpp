#include "esp_camera.h"
#include <WiFi.h>
#include "shakal_cam.h"
#include "FS.h"
#include "SPIFFS.h"
#include "index_html.h"
#include <ctime>
#include "img_converters.h"

// ============================================================================
// OUTPUT FORMAT CONFIGURATION
// ============================================================================
// To change output format, modify g_useJpegOutput in setup() function:
// - true  = JPEG output (faster, ~20-50ms encoding vs ~400ms for PNG)
// - false = PNG output (slower, but with 8-color palette)
// You can also toggle at runtime using POST /toggle-format endpoint
// ============================================================================

// Camera pin definitions
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1  // Software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

// Button and LED pin definitions
#define BUTTON_PIN 2      // Hardware button on GPIO2
#define BUILTIN_LED 33    // Built-in LED on ESP32-CAM (not the flash)
#define FLASH_LED 4       // Flash LED on GPIO4

// Wi-Fi settings
// const char* ssid = "shkubu";
// const char* password = "18061994";
// Alternative Wi-Fi configurations (commented out)
// const char* ssid = "Н's Galaxy A22";
// const char* password = "lublukolu";
// const char* ssid = "iPhone NS";
// const char* password = "lublukolu";
// const char* ssid = "Xiaomi_C909";
// const char* password = "9654215720";
const char* ssid = "FUCKWAR";
const char* password = "istand4PEACE";


// HTTP server on port 80
WiFiServer server(80);

// Global variables
int g_photoIndex = 0;
const unsigned long DEBOUNCE_DELAY = 100;  // Button debounce delay in milliseconds

// Streaming constants
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

/**
 * Generates a random 8-character hexadecimal hash for filenames
 * @return String with random hex hash
 */
String generateRandomHash() {
  static const char alphanum[] = "0123456789abcdef";
  String hash = "";
  for (int i = 0; i < 8; i++) {
    hash += alphanum[random(0, sizeof(alphanum) - 1)];
  }
  return hash;
}

/**
 * Blinks the built-in LED twice to indicate operation
 */
void blinkLedTwice() {
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
}

/**
 * Saves a photo to the SPIFFS filesystem with a random filename
 * 
 * @param data Pointer to image data
 * @param len Length of image data
 * @param filename Reference to store the generated filename
 */
void savePhotoToSPIFFS(const uint8_t* data, size_t len, String& filename) {
  if (!SPIFFS.exists("/photos")) SPIFFS.mkdir("/photos");
  
  // Use random hash for filename with appropriate extension
  String hash = generateRandomHash();
  String extension = g_useJpegOutput ? ".jpg" : ".png";
  filename = "/photos/img_" + hash + extension;
  
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (file) {
    file.write(data, len);
    file.close();
    Serial.println("Photo saved: " + filename);
  } else {
    Serial.println("Failed to open file for writing");
  }
}

/**
 * Sends a photo from SPIFFS to the client
 * 
 * @param client WiFiClient to send the photo to
 * @param filename Full path to the photo file
 */
void sendPhotoFromSPIFFS(WiFiClient& client, const String& filename) {
  Serial.println("Try to open file: " + filename);
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("Photo not found");
    return;
  }
  
  client.println("HTTP/1.1 200 OK");
  // Set content type based on file extension
  if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
    client.println("Content-Type: image/jpeg");
  } else {
    client.println("Content-Type: image/png");
  }
  client.println("Connection: close");
  client.println();
  
  uint8_t buf[512];
  while (file.available()) {
    size_t len = file.read(buf, sizeof(buf));
    client.write(buf, len);
  }
  file.close();
}

/**
 * Sends a gallery page with links to all photos
 * 
 * @param client WiFiClient to send the page to
 */
void sendGalleryPage(WiFiClient& client) {
  File root = SPIFFS.open("/photos");
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  size_t free = total > used ? total - used : 0;
  float percent = total > 0 ? (used * 100.0) / total : 0.0;
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Gallery</title></head><body><h1>Gallery</h1>");
  client.printf("<div><b>[ %u KB / %u KB ] (%.2f%%, FREE: %u KB)</b></div>\n", 
                (unsigned)(used/1024), (unsigned)(total/1024), 
                percent, (unsigned)(free/1024));
  
  File file = root.openNextFile();
  while (file) {
    String name = file.name();
    if (name.endsWith(".png") || name.endsWith(".jpg")) {
      size_t fsize = file.size();
      client.print("<div><a href='/photo/");
      client.print(name);
      client.print("'>");
      client.print(name);
      client.printf("</a> [ %u KB ]</div>\n", (unsigned)(fsize/1024));
    }
    file = root.openNextFile();
  }
  client.println("</body></html>");
}

/**
 * Sends a JSON list of all photos in the filesystem
 * 
 * @param client WiFiClient to send the JSON to
 */
void sendPhotoListJSON(WiFiClient& client) {
  File root = SPIFFS.open("/photos");
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  size_t free = total > used ? total - used : 0;
  float percent = total > 0 ? (used * 100.0) / total : 0.0;
  
  // Create a JSON string manually
  String json = "{\"files\":[";
  
  File file = root.openNextFile();
  bool isFirst = true;
  while (file) {
    String name = file.name();
    if (name.endsWith(".png") || name.endsWith(".jpg")) {
      if (!isFirst) {
        json += ",";
      }
      size_t fsize = file.size();
      json += "{\"name\":\"";
      json += name;
      json += "\",\"size\":";
      json += fsize;
      json += "}";
      isFirst = false;
    }
    file = root.openNextFile();
  }
  
  json += "],";
  json += "\"storage\":\"";
  json += String((unsigned)(used/1024)) + " KB / " + String((unsigned)(total/1024)) + " KB (";
  json += String(percent, 2) + "%, FREE: " + String((unsigned)(free/1024)) + " KB)";
  json += "\"}";
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println();
  client.print(json);
}

/**
 * Handles video streaming using multipart/x-mixed-replace
 * Continuously sends JPEG frames to create a video stream
 * 
 * @param client WiFiClient to stream video to
 */
void handleVideoStream(WiFiClient& client) {
  Serial.println("Starting video stream");
  
  // Send HTTP headers for multipart streaming
  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
  client.println("Access-Control-Allow-Methods: GET,POST,OPTIONS,DELETE,PUT");
  client.print("Content-Type: ");
  client.println(STREAM_CONTENT_TYPE);
  client.println();
  
  // Stream frames continuously
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed during streaming");
      break;
    }
    
    // Convert grayscale to JPEG if needed
    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool jpeg_converted = false;
    
    if (fb->format != PIXFORMAT_JPEG) {
      // Convert frame to JPEG
      jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
      if (!jpeg_converted) {
        Serial.println("JPEG conversion failed");
        esp_camera_fb_return(fb);
        break;
      }
    } else {
      jpg_buf = fb->buf;
      jpg_len = fb->len;
    }
    
    // Send multipart boundary
    client.print(STREAM_BOUNDARY);
    
    // Send part header with content length
    char part_buf[64];
    size_t hlen = snprintf(part_buf, 64, STREAM_PART, jpg_len);
    client.write((uint8_t*)part_buf, hlen);
    
    // Send JPEG data
    client.write(jpg_buf, jpg_len);
    
    // Clean up
    if (jpeg_converted && jpg_buf) {
      free(jpg_buf);
    }
    esp_camera_fb_return(fb);
    
    // Small delay to control frame rate
    delay(30); // ~33 FPS max
    
    // Check if client is still connected
    if (!client.connected()) {
      break;
    }
  }
  
  Serial.println("Video stream ended");
}

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("ESP32-CAM server is up at:");
  Serial.println("http://" + WiFi.localIP().toString());

  // Configure camera
  camera_config_t config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_GRAYSCALE,  // YUV422, GRAYSCALE, RGB565, JPEG
    .frame_size = FRAMESIZE_QVGA,        // QQVGA-UXGA, For ESP32 avoid sizes above QVGA when not JPEG

    // .jpeg_quality = 10,  // 0-63, for OV sensors lower number = higher quality
    .fb_count = 1,          // When JPEG mode is used, if fb_count > 1 the driver works in continuous mode
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  };

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x (%s)\n", err, esp_err_to_name(err));
    return;
  }

  // Initialize GPIO pins
  pinMode(FLASH_LED, OUTPUT);   // Flash LED
  pinMode(BUILTIN_LED, OUTPUT); // Built-in LED
  digitalWrite(BUILTIN_LED, HIGH); // Turn off built-in LED by default

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Initialize button with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Button initialized on GPIO " + String(BUTTON_PIN));

  // Find last photo index for sequential naming
  if (SPIFFS.exists("/photos")) {
    File root = SPIFFS.open("/photos");
    File file = root.openNextFile();
    int maxIdx = -1;
    while (file) {
      String name = file.name();
      if (name.startsWith("/photos/photo_") && name.endsWith(".png")) {
        int start = String("/photos/photo_").length();
        int end = name.length() - 4; // ".png"
        String numStr = name.substring(start, end);
        int idx = numStr.toInt();
        if (idx > maxIdx) maxIdx = idx;
      }
      file = root.openNextFile();
    }
    g_photoIndex = maxIdx + 1;
  }

  // Load color palette from SPIFFS or use default
  if (!loadPaletteFromSPIFFS()) {
    Serial.println("Using default color palette");
  } else {
    Serial.println("Color palette loaded from SPIFFS");
  }

  // Load block size from SPIFFS or use default
  if (!loadBlockSizeFromSPIFFS()) {
    Serial.println("Using default block size: " + String(g_blockSize));
  } else {
    Serial.println("Block size loaded from SPIFFS: " + String(g_blockSize));
  }

  // ============================================================================
  // SET OUTPUT FORMAT HERE - Change this line to switch between JPEG and PNG
  // ============================================================================
  g_useJpegOutput = false;  // Set to true for JPEG (faster), false for PNG (with palette)
  Serial.printf("Output format set to: %s\n", g_useJpegOutput ? "JPEG" : "PNG");
  if (g_useJpegOutput) {
    Serial.println("JPEG mode: Faster encoding (~20-50ms) but no color palette");
  } else {
    Serial.println("PNG mode: Slower encoding (~400ms) but with 8-color palette");
  }

  // Start HTTP server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Hardware button handling with debouncing
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  static bool buttonPressed = false;
  
  bool buttonState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  
  // Reset debounce timer if button state changed
  if (buttonState != lastButtonState) {
    lastDebounceTime = currentTime;
  }
  
  // Check if state is stable after debounce delay
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    // If button is pressed (LOW due to pull-up) and we haven't registered the press yet
    if (buttonState == LOW && !buttonPressed) {
      buttonPressed = true;
      Serial.println("Button pressed - taking photo");
      
      // Blink LED twice before capture
      blinkLedTwice();
      
      // First capture and discard (to stabilize camera)
      camera_fb_t *fb = esp_camera_fb_get();
      esp_camera_fb_return(fb);
      
      // Second capture for actual use
      fb = esp_camera_fb_get();
      if (fb) {
        unsigned long transformStartTime = millis();
        
        if (g_useJpegOutput) {
          Serial.println("Button capture - Using JPEG output format");
          // Generate JPEG data with pixelation
          uint8_t* jpegData = NULL;
          size_t jpegSize = 0;
          generateJPEGWithPixelationToRam(fb, &jpegData, &jpegSize);
          
          unsigned long transformEndTime = millis();
          Serial.printf("Button capture - Image transformation (pixelation + JPEG encoding) took: %lu ms\n", transformEndTime - transformStartTime);
          
          if (jpegData && jpegSize > 0) {
            unsigned long saveStartTime = millis();
            // Save photo to filesystem
            String filename;
            savePhotoToSPIFFS(jpegData, jpegSize, filename);
            unsigned long saveEndTime = millis();
            Serial.printf("Button capture - File save took: %lu ms\n", saveEndTime - saveStartTime);
            Serial.printf("Button capture - Total processing time: %lu ms\n", saveEndTime - transformStartTime);
            
            free(jpegData); // Free allocated memory
          } else {
            Serial.println("Failed to generate JPEG data");
          }
        } else {
          Serial.println("Button capture - Using PNG output format");
          // Generate PNG data with the current palette
          uint8_t* pngData = NULL;
          size_t pngSize = 0;
          generatePNGWithPaletteToRam(fb, &pngData, &pngSize);
          
          unsigned long transformEndTime = millis();
          Serial.printf("Button capture - Image transformation (pixelation + PNG encoding) took: %lu ms\n", transformEndTime - transformStartTime);
          
          if (pngData && pngSize > 0) {
            unsigned long saveStartTime = millis();
            // Save photo to filesystem
            String filename;
            savePhotoToSPIFFS(pngData, pngSize, filename);
            unsigned long saveEndTime = millis();
            Serial.printf("Button capture - File save took: %lu ms\n", saveEndTime - saveStartTime);
            Serial.printf("Button capture - Total processing time: %lu ms\n", saveEndTime - transformStartTime);
            
            free(pngData); // Free allocated memory
          } else {
            Serial.println("Failed to generate PNG data");
          }
        }
        
        esp_camera_fb_return(fb);
      } else {
        Serial.println("Camera capture failed");
      }
    }
    else if (buttonState == HIGH) {
      // Reset press flag when button is released
      buttonPressed = false;
    }
  }
  
  lastButtonState = buttonState;
  
  // HTTP server handling
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New Client.");
  
  // Wait for client request with timeout
  unsigned long timeout = millis() + 2000;
  while (!client.available() && millis() < timeout) {
    delay(1);
  }

  if (!client.available()) {
    Serial.println("Client timeout");
    client.stop();
    return;
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println("Request: " + request);
  client.read(); // Skip '\n'

  // Route handling
  if (request.indexOf("GET /capture") >= 0) {
    // Capture and send photo without saving
    unsigned long captureStartTime = millis();
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      client.println("HTTP/1.1 500 Internal Server Error");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("Camera capture failed");
      client.stop();
      return;
    }
    
    unsigned long captureEndTime = millis();
    Serial.printf("Camera capture took: %lu ms\n", captureEndTime - captureStartTime);
    Serial.printf("Got frame: %dx%d, format: %d\n", fb->width, fb->height, fb->format);

    // Send image directly to client without saving to file
    client.println("HTTP/1.1 200 OK");
    if (g_useJpegOutput) {
      client.println("Content-Type: image/jpeg");
    } else {
      client.println("Content-Type: image/png");
    }
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST");
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
    
    unsigned long transformStartTime = millis();
    if (g_useJpegOutput) {
      Serial.println("Using JPEG output format");
      sendNewJPEGWithPixelation(fb, client);
    } else {
      Serial.println("Using PNG output format");
      sendNewPNGWithPalette(fb, client);
    }
    unsigned long transformEndTime = millis();
    
    Serial.printf("Image transformation (pixelation + %s encoding) took: %lu ms\n", 
                  g_useJpegOutput ? "JPEG" : "PNG", transformEndTime - transformStartTime);
    Serial.printf("Total processing time: %lu ms\n", transformEndTime - captureStartTime);
    
    esp_camera_fb_return(fb);

    client.stop();
    Serial.printf("%s sent to client (no file save)\n", g_useJpegOutput ? "JPEG" : "PNG");
    return;
  } else if (request.indexOf("GET /gallery") >= 0) {
    // Gallery page
    String html = getGalleryHTML();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.print("Content-Length: ");
    client.println(html.length());
    client.println("Connection: close");
    client.println();
    client.print(html);
    client.stop();
    return;
  } else if (request.indexOf("GET /photo-list") >= 0) {
    // JSON API endpoint for photo listing
    sendPhotoListJSON(client);
    client.stop();
    return;
  } else if (request.indexOf("POST /delete-all-photos") >= 0) {
    // Delete all photos
    Serial.println("Deleting all photos...");
    
    // Step 1: Collect all filenames in an array
    const int MAX_FILES = 100; // Maximum number of files to delete
    String filesToDelete[MAX_FILES];
    int fileCount = 0;
    
    File root = SPIFFS.open("/photos");
    
    // Collect all filenames first
    File file = root.openNextFile();
    while (file && fileCount < MAX_FILES) {
      String name = file.name();
      if (name.endsWith(".png") || name.endsWith(".jpg")) {
        filesToDelete[fileCount] = name;
        fileCount++;
      }
      file = root.openNextFile();
    }
    
    root.close();
    
    // Step 2: Delete each file
    int deletedCount = 0;
    
    Serial.printf("Found %d files to delete\n", fileCount);
    
    for (int i = 0; i < fileCount; i++) {
      String fullPath = filesToDelete[i];
      Serial.println("Trying to delete: " + fullPath);
      
      // Just use the full path directly for delete
      if (SPIFFS.remove(fullPath)) {
        deletedCount++;
      }
    }
    
    // Send JSON response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    
    String response = "{\"success\":true,\"deletedFiles\":" + String(deletedCount) + "}";
    client.println(response.length());
    client.println();
    client.print(response);
    client.stop();
    return;
  } else if (request.indexOf("GET /flash") >= 0) {
    // Toggle flash LED
    static bool flashState = false;
    flashState = !flashState;
    digitalWrite(FLASH_LED, flashState ? HIGH : LOW);
    delay(100); // Let the flash stabilize
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<html><body></body></html>");
    client.stop();
    return;
  } else if (request.indexOf("GET /photo/") >= 0) {
    // Serve specific photo
    int idx = request.indexOf("/photo/");
    if (idx >= 0) {
      int spaceIdx = request.indexOf(' ', idx + 7);
      String fname = (spaceIdx > 0) ? request.substring(idx + 7, spaceIdx) : request.substring(idx + 7);
      fname.trim();
      int lastSlash = fname.lastIndexOf('/');
      if (lastSlash >= 0) {
        fname = fname.substring(lastSlash + 1);
      }
      sendPhotoFromSPIFFS(client, "/photos/" + fname);
      client.stop();
      return;
    }
  } else if (request.indexOf("GET /settings") >= 0) {
    // Settings page
    String html = getSettingsHTML();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.print("Content-Length: ");
    client.println(html.length());
    client.println("Connection: close");
    client.println();
    client.print(html);
    client.stop();
    return;
  } else if (request.indexOf("GET /get-palette") >= 0) {
    // Get current palette as JSON
    String paletteJson = getPaletteJSON();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(paletteJson.length());
    client.println();
    client.print(paletteJson);
    client.stop();
    return;
  } else if (request.indexOf("POST /save-palette") >= 0) {
    // Save palette settings
    String jsonData = "";
    while (client.available()) {
      char c = client.read();
      jsonData += c;
    }
    
    bool success = false;
    if (jsonData.length() > 0) {
      success = updatePaletteFromJSON(jsonData);
      if (success) {
        savePaletteToSPIFFS();
      }
    }
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    
    String response = "{\"success\":" + String(success ? "true" : "false") + "}";
    client.println(response.length());
    client.println();
    client.print(response);
    client.stop();
    return;
  } else if (request.indexOf("POST /reset-palette") >= 0) {
    // Reset palette to default
    resetPaletteToDefault();
    String paletteJson = getPaletteJSON();
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(paletteJson.length());
    client.println();
    client.print(paletteJson);
    client.stop();
    return;
  } else if (request.indexOf("GET /get-block-size") >= 0) {
    // Get current block size as JSON
    String blockSizeJson = getBlockSizeJSON();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(blockSizeJson.length());
    client.println();
    client.print(blockSizeJson);
    client.stop();
    return;
  } else if (request.indexOf("POST /save-block-size") >= 0) {
    // Save block size settings
    String jsonData = "";
    while (client.available()) {
      char c = client.read();
      jsonData += c;
    }
    
    bool success = false;
    if (jsonData.length() > 0) {
      success = updateBlockSizeFromJSON(jsonData);
      if (success) {
        saveBlockSizeToSPIFFS();
      }
    }
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    
    String response = "{\"success\":" + String(success ? "true" : "false") + "}";
    client.println(response.length());
    client.println();
    client.print(response);
    client.stop();
    return;
  } else if (request.indexOf("POST /reset-block-size") >= 0) {
    // Reset block size to default
    resetBlockSizeToDefault();
    String blockSizeJson = getBlockSizeJSON();
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(blockSizeJson.length());
    client.println();
    client.print(blockSizeJson);
    client.stop();
    return;
  } else if (request.indexOf("POST /reboot") >= 0) {
    // Reboot ESP32 system
    Serial.println("Reboot requested via POST endpoint");
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    
    String response = "{\"success\":true,\"message\":\"Rebooting ESP32...\"}";
    client.println(response.length());
    client.println();
    client.print(response);
    client.stop();
    
    // Small delay to ensure response is sent before reboot
    delay(100);
    
    // Trigger ESP32 system restart
    ESP.restart();
    return;
  } else if (request.indexOf("GET /stream") >= 0) {
    // Video streaming endpoint
    handleVideoStream(client);
    client.stop();
    return;
  } else if (request.indexOf("GET /performance-test") >= 0) {
    // Performance test endpoint - captures image and returns timing data as JSON
    unsigned long testStartTime = millis();
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      client.println("HTTP/1.1 500 Internal Server Error");
      client.println("Content-Type: application/json");
      client.println("Access-Control-Allow-Origin: *");
      client.println("Connection: close");
      client.println();
      client.println("{\"error\":\"Camera capture failed\"}");
      client.stop();
      return;
    }
    
    unsigned long captureTime = millis() - testStartTime;
    int imageWidth = fb->width;
    int imageHeight = fb->height;
    
    // Test PNG transformation to RAM
    unsigned long pngTransformStart = millis();
    uint8_t* pngData = NULL;
    size_t pngSize = 0;
    generatePNGWithPaletteToRam(fb, &pngData, &pngSize);
    unsigned long pngTransformEnd = millis();
    
    if (pngData) {
      free(pngData);
    }
    
    // Test JPEG transformation to RAM
    unsigned long jpegTransformStart = millis();
    uint8_t* jpegData = NULL;
    size_t jpegSize = 0;
    generateJPEGWithPixelationToRam(fb, &jpegData, &jpegSize);
    unsigned long jpegTransformEnd = millis();
    
    if (jpegData) {
      free(jpegData);
    }
    
    esp_camera_fb_return(fb);
    
    unsigned long totalTestTime = millis() - testStartTime;
    
    // Create JSON response with timing data
    String jsonResponse = "{";
    jsonResponse += "\"capture_time_ms\":" + String(captureTime) + ",";
    jsonResponse += "\"png_transform_time_ms\":" + String(pngTransformEnd - pngTransformStart) + ",";
    jsonResponse += "\"jpeg_transform_time_ms\":" + String(jpegTransformEnd - jpegTransformStart) + ",";
    jsonResponse += "\"current_format\":\"" + String(g_useJpegOutput ? "jpeg" : "png") + "\",";
    jsonResponse += "\"total_test_time_ms\":" + String(totalTestTime) + ",";
    jsonResponse += "\"image_width\":" + String(imageWidth) + ",";
    jsonResponse += "\"image_height\":" + String(imageHeight) + ",";
    jsonResponse += "\"block_size\":" + String(g_blockSize) + ",";
    jsonResponse += "\"timestamp\":" + String(millis());
    jsonResponse += "}";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(jsonResponse.length());
    client.println();
    client.print(jsonResponse);
    client.stop();
    return;
  } else if (request.indexOf("POST /toggle-format") >= 0) {
    // Toggle between JPEG and PNG output format
    g_useJpegOutput = !g_useJpegOutput;
    
    String response = "{\"success\":true,\"current_format\":\"" + String(g_useJpegOutput ? "jpeg" : "png") + "\"}";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(response.length());
    client.println();
    client.print(response);
    
    Serial.printf("Output format toggled to: %s\n", g_useJpegOutput ? "JPEG" : "PNG");
    
    client.stop();
    return;
  } else if (request.indexOf("GET /get-format") >= 0) {
    // Get current output format
    String response = "{\"current_format\":\"" + String(g_useJpegOutput ? "jpeg" : "png") + "\"}";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(response.length());
    client.println();
    client.print(response);
    client.stop();
    return;
  } else {
    // Default: serve main HTML page
    String html = getIndexHTML();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.print("Content-Length: ");
    client.println(html.length());
    client.println("Connection: close");
    client.println();
    client.print(html);
    client.stop();
    return;
  }

  client.stop();
  Serial.println("Client disconnected.");
}
