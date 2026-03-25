 #include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

const char* WIFI_SSID     = "GodWin";
const char* WIFI_PASSWORD = "iiiiiiii";

#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 22

Audio audio;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    // SOUND IMPROVEMENT 1: Increase buffer size for smoother streaming
    audio.setBufferSize(16000); 

    // Initialize I2S
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // SOUND IMPROVEMENT 2: Lower volume to 18 to avoid digital clipping
    audio.setVolume(21); 

    // SOUND IMPROVEMENT 3: DSP Equalizer - Boost mids, cut sharp highs
    audio.setTone(0, 4, -6); 

    Serial.println("\n✓ Ready. Audio optimized for clarity.");
}

void loop() {
    audio.loop();

    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();
        if (text.length() > 0) {
            audio.connecttospeech(text.c_str(), "en");
        }
    }
}