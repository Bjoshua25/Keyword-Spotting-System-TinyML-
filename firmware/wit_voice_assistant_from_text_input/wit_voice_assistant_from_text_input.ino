#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ESP8266SAM.h"
#include "AudioOutputI2S.h"

const char* ssid = "GodWin";
const char* password = "iiiiiiii";
const char* token = "5UMB2NTKO7IAFFESKLO5FHFSWOIKA5HL";

#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 22

AudioOutputI2S *out;
ESP8266SAM *sam;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    sam = new ESP8266SAM();
    sam->SetPitch(90);
    sam->SetSpeed(100);
    Serial.println("✓ Text-to-Entity Ready. Type your question...");
}

void processEntities(DynamicJsonDocument& doc) {
    // Check if "name:name" entity exists
    if (doc["entities"].containsKey("name:name")) {
        sam->Say(out, "My name is Robo Dog.");
    } 
    // Check if "you:you" entity exists
    else if (doc["entities"].containsKey("you:you")) {
        sam->Say(out, "I am doing great today!");
    }
    // Check for "time:time"
    else if (doc["entities"].containsKey("time:time")) {
        sam->Say(out, "I do not have a clock yet.");
    }
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.replace(" ", "%20");

        HTTPClient http;
        String url = "https://api.wit.ai/message?v=20240304&q=" + input;
        http.begin(url);
        http.addHeader("Authorization", "Bearer " + String(token));
        
        int httpCode = http.GET();
        if (httpCode == 200) {
            String payload = http.getString();
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, payload);
            processEntities(doc);
        }
        http.end();
    }
}