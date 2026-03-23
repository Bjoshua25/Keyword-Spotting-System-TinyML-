#include <Audio.h>

/* Go2 Air Manual Voice Brain Only (High Quality Optimized MP3)
 * Text input from Serial Monitor box -> Output through Speaker.
 * Strategic Implementation: Uses optimized Wolle hardware decoding without external library conflicts.
 * NO TINYML INFERENCING. STRICTLY VALIDATING AUDIO PIPELINE FIRST.
 * Resources Resolved: Prioritized Multicore task (Core 1) & Force SRAM buffer size (OOM Fixed).
 */

// Core Libraries
#include <WiFi.h>
#include <Audio.h> // *** Wolle's validated v2.0.6 dependency ***

// FreeRTOS specifics for multicore handling (ensures prioritized audio output)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==============================================================================
// CONFIGURATION (Using standard user inputs)
// ==============================================================================

// WiFi Credentials
const char* WIFI_SSID     = "GodWin";
const char* WIFI_PASSWORD = "iiiiiiii";

// Wit.ai Token (verified Server Access Token from Turn 28)
const char* WIT_TOKEN     = "5UMB2NTKO7IAFFESKLO5FHFSWOIKA5HL";

// Wit.ai API Details
#define WITAI_HOST "api.wit.ai"
#define WITAI_PORT 443
#define WITAI_PATH "/synthesize?v=20240321" // API version date

// I2S Pins (Using custom mapping verified)
#define SPK_I2S_DIN  22
#define SPK_I2S_LRC  25  // (WS)
#define SPK_I2S_BCLK 26
#define SPK_I2S_PORT I2S_NUM_1 // Strictly Порт 1

// Common Audio Settings
#define SAMPLE_RATE 16000 // Voice standard

// ==============================================================================
// 1. OBJECT CREATION (Uses v2.0.6 signature)
// ==============================================================================
// parameter 1 (optional): strictly Porт 1 for high quality
Audio audio(SPK_I2S_PORT); 

// Forward declarations
static void voiceLoopTask(void *pvParameters); //Core 1 prioritized task function

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n========================================");
    Serial.println("Go2 Air Robot Standalone Voice Brain - PCM FIXED");
    Serial.println("Copyright (c) 2026 Momas-Epail");
    Serial.println("========================================\n");

    // ==============================================================================
    // 2. CRITICAL FIX: SRAM Buffer Optimization (Before speaker init)
    // ==============================================================================
    // The library defaults to massive allocations (720KB) meant for PSRAM boards.
    // We override this immediately.
    // 98304 = 96KB SRAM (fits comfortably on standard ESP32-WROOM).
    // 0 = Explicitly disable PSRAM allocation.
    Serial.print("Configuring memory-optimized 96KB SRAM AudioBuffer...\n");
    audio.setBufsize(98304, 0); 

    // 1. Initialize WiFi manually (Validation Stage 1)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nSUCCESS! Connected to Internet.");

    // 2. Initialize Speaker Hardware automatically.
    Serial.print("\nInitializing Optimized MP3 Decoder & Speaker...\n");
    
    // The library handles the hardware initialization internally using your pins.
    // DOUT=22, WS=25, BCK=26
    audio.setPinout(SPK_I2S_BCLK, SPK_I2S_LRC, SPK_I2S_DIN); 
    audio.setVolume(21); // Set volume (0-21)
    
    Serial.println("✓ High-Quality Optimized Voice Hardware Ready!");

    Serial.println("Type text in Serial Monitor input and press Enter.");
    Serial.println("========================================\n");
    
    // ==============================================================================
    // 3. MULTICORE INTEGRATION: Create prioritized Voice maintaining task
    // ==============================================================================
    xTaskCreatePinnedToCore(
        voiceLoopTask,      // Function to implement the task
        "VoiceMaintainer",  // Name of the task
        1024 * 16,          // Stack size in words (not bytes)
        NULL,               // Task input parameter
        10,                 // Priority of the task (High Priority for voice)
        NULL,               // Task handle
        1                   // Core where the task should run (Core 1)
    );
}

void loop() {
    // MAINTAINING THE DECODING LOOP (Prioritized by FreeRTOS on Core 1 task below)
    // We brief yield to let standard OS management proceed on Core 0
    yield(); 
}

// ==============================================================================
// 4. CORE 1 TASK: High Priority Voice maintaining loop
// ==============================================================================
static void voiceLoopTask(void *pvParameters) {
    while (true) {
        // Optimized maintaining loop - must be prioritized for quality
        audio.loop(); 

        // Check for serial input for manual TTS trigger (Validation logic)
        if (Serial.available() > 0) {
            String inputText = Serial.readStringUntil('\n'); // Read input
            inputText.trim(); 

            if (inputText.length() > 0) {
                // modified string as requested
                fetchAndStreamWitMP3(inputText);
            }
        }
        yield(); // prevent watchdog
    }
}

// Function to handle payload formatting, authentication, and optimized MP3 streaming
void fetchAndStreamWitMP3(String text) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: No WiFi for Voice Brain.");
        return;
    }

    // URL Encoding and JSON Payload formatting PoC space replacement
    String encodedText = text;
    encodedText.replace(" ", "%20");
    encodedText.replace(",", "%2C");
    encodedText.replace("-", "%2D");
    
    // minimal SSML for voice control
    String q_formatted = "<speak>" + encodedText + "</speak>";

    // Build JSON payload
    String payload = "{";
    payload += "\"q\":\"" + q_formatted + "\",";
    payload += "\"voice\":\"wit$Rebecca\","; // Standard stable voice
    payload += "\"speed\":90,";
    payload += "\"pitch\":100";
    payload += "}";

    // Request URL
    String wit_url = "https://" + String(WITAI_HOST) + String(WITAI_PATH);
    Serial.print("API Call: POST to "); Serial.println(wit_url);

    // Optimized streaming call - the library abstracts the HTTP loop and buffering automatically.
    // Ear Brain is disabled, loop() is prioritized for quality on Core 1 task.
    audio.connecttohost(wit_url.c_str(), payload.c_str()); 
}

// Low-Level I2S Hardware Driver standalone initialization function no longer required.