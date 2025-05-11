#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>
#include "SPIFFS.h"

// Function to load HTML content from SPIFFS
String loadIndexHTML() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return "";
    }
    
    File file = SPIFFS.open("/web/index.min.html", "r");
    if (!file) {
        Serial.println("Failed to open index.min.html");
        return "";
    }
    
    String content = file.readString();
    file.close();
    return content;
}

// Get HTML content as const char* (compatible with existing code)
const char* getIndexHTML() {
    static String content = loadIndexHTML();
    return content.c_str();
}

#endif // INDEX_HTML_H
