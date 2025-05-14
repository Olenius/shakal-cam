#include "esp_camera.h"
#include <WiFi.h>
#include "shakal_cam.h"
#include "FS.h"
#include "SPIFFS.h"
#include "index_html.h"
#include <ctime>

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
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

// Определение пина кнопки
#define BUTTON_PIN 2  // Меняем на GPIO2, который часто используется для кнопок
// Определение пина встроенного светодиода
#define BUILTIN_LED 33  // Встроенный светодиод на ESP32-CAM (не вспышка)

// Настройки Wi-Fi
// const char* ssid = "Н's Galaxy A22";
// const char* password = "lublukolu";
const char* ssid = "shkubu";
const char* password = "18061994";
// const char* ssid = "iPhone NS";
// const char* password = "lublukolu";

WiFiServer server(80);

void startCameraServer();

int g_photoIndex = 0;

// Функция генерации случайного хеша для имени файла
String generateRandomHash() {
  static const char alphanum[] = "0123456789abcdef";
  String hash = "";
  for (int i = 0; i < 8; i++) {
    hash += alphanum[random(0, sizeof(alphanum) - 1)];
  }
  return hash;
}

// Функция для двойного мигания светодиодом
void blinkLedTwice() {
  // Первая вспышка
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  
  // Вторая вспышка
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
}

void savePhotoToSPIFFS(const uint8_t* data, size_t len, String& filename) {
  if (!SPIFFS.exists("/photos")) SPIFFS.mkdir("/photos");
  
  // Используем случайный хеш для имени файла
  String hash = generateRandomHash();
  filename = "/photos/img_" + hash + ".png";
  
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (file) {
    file.write(data, len);
    file.close();
    Serial.println("Photo saved: " + filename);
  } else {
    Serial.println("Failed to open file for writing");
  }
}

void sendPhotoFromSPIFFS(WiFiClient& client, const String& filename) {
  Serial.println("Try to open file: " + filename);
  File file = SPIFFS.open(filename, FILE_READ); // исправлено: filename уже содержит полный путь
  if (!file) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("Photo not found");
    return;
  }
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/png");
  client.println("Connection: close");
  client.println();
  uint8_t buf[512];
  while (file.available()) {
    size_t len = file.read(buf, sizeof(buf));
    client.write(buf, len);
  }
  file.close();
}

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
  client.printf("<div><b>[ %u KB / %u KB ] (%.2f%%, FREE: %u KB)</b></div>\n", (unsigned)(used/1024), (unsigned)(total/1024), percent, (unsigned)(free/1024));
  File file = root.openNextFile();
  while (file) {
    String name = file.name();
    if (name.endsWith(".png")) {
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

void sendPhotoListJSON(WiFiClient& client) {
  File root = SPIFFS.open("/photos");
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  size_t free = total > used ? total - used : 0;
  float percent = total > 0 ? (used * 100.0) / total : 0.0;
  
  // Create a JSON string manually since ArduinoJson library might not be available
  String json = "{\"files\":[";
  
  File file = root.openNextFile();
  bool isFirst = true;
  while (file) {
    String name = file.name();
    if (name.endsWith(".png")) {
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

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("ESP32-CAM IP address: ");
  Serial.println(WiFi.localIP());

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

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    // .jpeg_quality = 10, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  };


  // Инициализация камеры
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x (%s)\n", err, esp_err_to_name(err));
    return;
  }

  pinMode(4, OUTPUT); // GPIO4 — flash LED
  pinMode(BUILTIN_LED, OUTPUT); // Инициализируем встроенный светодиод
  digitalWrite(BUILTIN_LED, HIGH); // Выключаем встроенный светодиод по умолчанию

  // // Инициализация SPIFFS
  // if (true) {
  //   SPIFFS.format();
  // }
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Инициализация пина кнопки
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Button initialized on GPIO " + String(BUTTON_PIN));

  // --- Find last photo index ---
  if (SPIFFS.exists("/photos")) {
    File root = SPIFFS.open("/photos");
    File file = root.openNextFile();
    int maxIdx = -1;
    while (file) {
      String name = file.name();
      if (name.startsWith("/photos/photo_") && name.endsWith(".png")) {
        int start = String("/photos/photo_").length();
        int end = name.length() - 4; // ".bmp"
        String numStr = name.substring(start, end);
        int idx = numStr.toInt();
        if (idx > maxIdx) maxIdx = idx;
      }
      file = root.openNextFile();
    }
    g_photoIndex = maxIdx + 1;
  }

  // Загружаем настройки палитры из SPIFFS при старте
  // Если не удалось загрузить, будет использована палитра по умолчанию
  if (!loadPaletteFromSPIFFS()) {
    Serial.println("Using default color palette");
  } else {
    Serial.println("Color palette loaded from SPIFFS");
  }

  // Запуск сервера
  server.begin();
}

void loop() {
  // Проверяем нажатие аппаратной кнопки для захвата фото
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  static unsigned long lastDebugTime = 0;
  static bool buttonPressed = false;
  
  bool buttonState = digitalRead(BUTTON_PIN);

  unsigned long currentTime = millis();
  
  // Если состояние кнопки изменилось, сбрасываем таймер дребезга
  if (buttonState != lastButtonState) {
    lastDebounceTime = currentTime;
  }
  
  // Проверяем, прошло ли достаточно времени после последнего изменения состояния
  if ((currentTime - lastDebounceTime) > 100) {  // Увеличил время антидребезга до 100 мс
    // Если состояние стабилизировалось и кнопка нажата (LOW)
    if (buttonState == LOW && !buttonPressed) {
      buttonPressed = true;
      Serial.println("Button pressed - taking photo");
      
      // Мигаем светодиодом дважды перед захватом фото
      blinkLedTwice();
      
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) {
        // Сгенерировать PNG данные
        uint8_t* pngData = NULL;
        size_t pngSize = 0;
        generatePNGWithPaletteToRam(fb, &pngData, &pngSize);
        
        if (pngData && pngSize > 0) {
          // Сохраняем фото в SPIFFS
          String filename;
          savePhotoToSPIFFS(pngData, pngSize, filename);
          free(pngData); // Освобождаем память
        } else {
          Serial.println("Failed to generate PNG data");
        }
        
        esp_camera_fb_return(fb);
      } else {
        Serial.println("Camera capture failed");
      }
    }
    else if (buttonState == HIGH) {
      // Сбрасываем флаг нажатия только когда кнопка отпущена
      buttonPressed = false;
    }
  }
  
  lastButtonState = buttonState;
  
  // Обработка HTTP запросов
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("New Client.");
  // Ждём, пока клиент отправит запрос (с небольшим таймаутом)
  unsigned long timeout = millis() + 2000;
  while (!client.available() && millis() < timeout) {
    delay(1);
  }

  if (!client.available()) {
    Serial.println("Client timeout");
    client.stop();
    return;
  }

  String request = client.readStringUntil('\r');
  Serial.println("Request: " + request);
  client.read(); // Пропустить '\n'

  // Захват фото
  if (request.indexOf("GET /capture") >= 0) {
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
  
    Serial.printf("Got frame: %dx%d, format: %d\n", fb->width, fb->height, fb->format);

    // --- Только отправка PNG клиенту, без сохранения в файл ---
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/png");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST");
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
    
    sendNewPNGWithPalette(fb, client);
    esp_camera_fb_return(fb);

    client.stop();
    Serial.println("PNG sent to client (no file save)");
    return;
  } else if (request.indexOf("GET /gallery") >= 0) {
    // Use the new gallery HTML function
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
    // Handle the new JSON API endpoint for photo listing
    sendPhotoListJSON(client);
    client.stop();
    return;
  } else if (request.indexOf("POST /delete-all-photos") >= 0) {
    // Handle delete all photos request
    Serial.println("Deleting all photos...");
    
    // Step 1: Collect all png filenames in an array
    const int MAX_FILES = 100; // Maximum number of files to delete
    String filesToDelete[MAX_FILES];
    int fileCount = 0;
    
    File root = SPIFFS.open("/photos");
    
    // First collect all filenames
    File file = root.openNextFile();
    while (file && fileCount < MAX_FILES) {
      String name = file.name();
      if (name.endsWith(".png")) {
        // Store the full path to each file
        filesToDelete[fileCount] = name;
        fileCount++;
      }
      file = root.openNextFile();
    }
    
    // Close all open files and the directory
    root.close();
    
    // Step 2: Delete each file one by one
    int deletedCount = 0;
    
    Serial.printf("Found %d files to delete\n", fileCount);
    
    // Simple direct deletion approach
    for (int i = 0; i < fileCount; i++) {
      String fullPath = filesToDelete[i];
      Serial.println("Trying to delete: " + fullPath);
      
      SPIFFS.remove("/photos/" + filesToDelete[i]);
      deletedCount++;
    }
    
    // Send simple JSON response
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
    static bool flashState = false;
    flashState = !flashState;
    digitalWrite(4, flashState ? HIGH : LOW);
    delay(100); // дать вспышке загореться
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<html><body></body></html>");
    client.stop();
    return;
  } else if (request.indexOf("GET /photo/") >= 0) {
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
    // Страница настроек
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
    // Получить текущую палитру в формате JSON
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
    // Сохранение палитры
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
    // Сбросить палитру к значениям по умолчанию
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
  } else {
    // Отправка встроенного HTML из index_html.h
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

  delay(1); // небольшая задержка
  client.stop();
  Serial.println("Client disconnected.");
}
