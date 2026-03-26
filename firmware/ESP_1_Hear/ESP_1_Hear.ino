#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

// ==================== CONFIGURATION ====================
const char* ssid = "GodWin";
const char* password = "iiiiiiii";
// Ensure "Bearer " is included for the Speech API
const char* token = "Bearer 5UMB2NTKO7IAFFESKLO5FHFSWOIKA5HL";

// Mic Pins (INMP441)
#define MIC_WS  33 
#define MIC_SD  32
#define MIC_SCK 14

// LED Pin
#define RECORD_LED 4 

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200); // UART to ESP 2
    
    pinMode(RECORD_LED, OUTPUT); 
    digitalWrite(RECORD_LED, LOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✓ WiFi Connected");
    
    setupMic();
    Serial.println("✓ ESP 1 Ready. Press 'R' to record.");
}

// ==================== I2S MIC SETUP ====================
void setupMic() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000, // Wit.ai standard
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = MIC_SCK,
        .ws_io_num = MIC_WS,
        .data_out_num = -1, // Not used for recording
        .data_in_num = MIC_SD
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ==================== CORE LOGIC ====================
void recordAndSend() {
    HTTPClient http;
    http.setTimeout(10000); 
    http.begin("https://api.wit.ai/speech?v=20240304");
    http.addHeader("Authorization", token);
    http.addHeader("Content-Type", "audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little");

    int httpCode = http.sendRequest("POST", (uint8_t*)NULL, 0); 
    WiFiClient* stream = http.getStreamPtr();
    
    uint8_t buffer[1024];
    size_t bytes_read;
    unsigned long start = millis();
    
    digitalWrite(RECORD_LED, HIGH); 
    Serial.println("\n[1] STATUS: Listening (3s)...");

    while (millis() - start < 3000) {
        i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
        if (bytes_read > 0 && stream->connected()) {
            stream->write(buffer, bytes_read);
            yield(); 
        }
    }

    digitalWrite(RECORD_LED, LOW); 
    Serial.println("[2] STATUS: Recording Finished. Waiting for Wit.ai...");

    String response = http.getString();
    http.end();
    
    // DEBUG: Print the raw response to see if Wit.ai actually heard you
    Serial.println("[3] RAW RESPONSE FROM WIT.AI:");
    Serial.println(response);

    int finalIndex = response.lastIndexOf("{\"entities\"");
    if (finalIndex != -1) {
        DynamicJsonDocument doc(4096); // Increased size for deep parsing
        deserializeJson(doc, response.substring(finalIndex));
        
        if (doc["is_final"] == true) {
            String recognizedText = doc["text"].as<String>();
            Serial.println("[4] FINAL TEXT: " + recognizedText);

            // LOGGING ENTITY DETECTION
            if (doc["entities"].containsKey("name:name")) {
                Serial.println("[5] ENTITY FOUND: name -> Sending 'N' to ESP 2");
                Serial2.print('N'); 
            } 
            else if (doc["entities"].containsKey("you:you")) {
                Serial.println("[5] ENTITY FOUND: you -> Sending 'H' to ESP 2");
                Serial2.print('H'); 
            }
            else if (doc["entities"].containsKey("time:time")) {
                Serial.println("[5] ENTITY FOUND: time -> Sending 'T' to ESP 2");
                Serial2.print('T'); 
            } else {
                Serial.println("[5] ERROR: No matching Entity found in JSON.");
            }
        }
    } else {
        Serial.println("[ERROR] No JSON Understanding found. Check Mic audio quality.");
    }
}



void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'R' || c == 'r') {
            recordAndSend();
        }
    }
}