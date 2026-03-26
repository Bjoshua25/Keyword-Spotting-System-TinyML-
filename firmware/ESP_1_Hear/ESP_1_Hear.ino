#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <driver/i2s.h>
#include <ArduinoJson.h>

// --- Configuration ---
const char* ssid = "GodWin";
const char* password = "iiiiiiii";
const char* wit_token = "5UMB2NTKO7IAFFESKLO5FHFSWOIKA5HL";

#define I2S_SCK  14
#define I2S_WS   33
#define I2S_SD   32
#define I2S_PORT I2S_NUM_0
#define RECORDING_LED 4
#define SAMPLE_RATE 16000
#define CHUNK_SIZE 1024  

struct IntentResponse {
    String text;
    String intent;
    String entity_value;
    float confidence;
    int httpStatus;
};

WiFiClientSecure client;

void setupI2S() {
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK, .ws_io_num = I2S_WS,
        .data_out_num = -1, .data_in_num = I2S_SD
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi Connected!");
    client.setInsecure(); 
}

IntentResponse performRecognition() {
    IntentResponse result = {"", "None", "None", 0.0, 0};
    
    if (!client.connect("api.wit.ai", 443)) {
        Serial.println("Connection failed");
        return result;
    }

    digitalWrite(RECORDING_LED, HIGH); 
    Serial.println(">>> LISTENING...");

    client.println("POST /speech?v=20240304 HTTP/1.1");
    client.println("Host: api.wit.ai");
    client.printf("Authorization: Bearer %s\n", wit_token);
    client.println("Content-Type: audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little");
    client.println("Transfer-Encoding: chunked");
    client.println();

    unsigned long start_time = millis();
    int16_t i2s_buffer[CHUNK_SIZE];
    size_t bytes_read;

    while (millis() - start_time < 4000) { 
        i2s_read(I2S_PORT, &i2s_buffer, sizeof(i2s_buffer), &bytes_read, portMAX_DELAY);
        if (bytes_read > 0) {
            int samples_read = bytes_read / 2;
            for (int i = 0; i < samples_read; i++) {
                i2s_buffer[i] = i2s_buffer[i] << 4; // High gain for INMP441
            }
            client.printf("%X\r\n", bytes_read);
            client.write((uint8_t*)i2s_buffer, bytes_read);
            client.print("\r\n");
        }
    }

    client.print("0\r\n\r\n"); 
    digitalWrite(RECORDING_LED, LOW); 
    Serial.println(">>> PROCESSING...");

    // 1. Parse HTTP Headers
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1")) {
            result.httpStatus = line.substring(9, 12).toInt();
        }
        if (line == "\r") break; 
    }

    // 2. Capture and Print RAW JSON Body
    String rawResponse = "";
    while (client.available()) {
        char c = client.read();
        rawResponse += c;
    }
    
    Serial.println("--- RAW SERVER RESPONSE ---");
    Serial.println(rawResponse);
    Serial.println("---------------------------");

    // 3. Parse JSON from the captured string
    if (result.httpStatus == 200 && rawResponse.length() > 0) {
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, rawResponse);
        
        if (!error) {
            result.text = doc["text"].as<String>();
            if (doc["intents"].size() > 0) {
                result.intent = doc["intents"][0]["name"].as<String>();
                result.confidence = doc["intents"][0]["confidence"].as<float>();
            }
            JsonObject entities = doc["entities"].as<JsonObject>();
            for (JsonPair p : entities) {
                result.entity_value = p.value()[0]["value"].as<String>();
                break; 
            }
        }
    }

    client.stop();
    return result;
}

void setup() {
    Serial.begin(115200);
    pinMode(RECORDING_LED, OUTPUT);
    connectToWiFi();
    setupI2S();
    Serial.println("Ready. Send 'r' to record.");
}

void loop() {
    if (Serial.available() > 0) {
        if (Serial.read() == 'r') {
            IntentResponse resp = performRecognition();
            Serial.printf("Final Intent: %s\n", resp.intent.c_str());
        }
    }
}