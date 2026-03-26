#include <Arduino.h>
#include "ESP8266SAM.h"
#include "AudioOutputI2S.h"

#define SPK_BCLK 26
#define SPK_LRC  25
#define SPK_DIN  22

AudioOutputI2S *out;
ESP8266SAM *sam;

void setup() {
    Serial.begin(115200);   // Debug to PC
    Serial2.begin(115200);  // Input from ESP 1
    
    out = new AudioOutputI2S();
    out->SetPinout(SPK_BCLK, SPK_LRC, SPK_DIN);
    sam = new ESP8266SAM();
    
    sam->SetPitch(90);
    sam->SetSpeed(100);
    
    Serial.println("✓ ESP 2 (Mouth) Ready. Waiting for UART command...");
}

void loop() {
    if (Serial2.available()) {
        char cmd = Serial2.read();
        
        // LOGGING UART RECEIPT
        Serial.print("\n[UART] Received Command: ");
        Serial.println(cmd);

        yield();
        delay(10);

        if (cmd == 'N') {
            Serial.println("[SAM] Speaking: Name Response");
            sam->Say(out, "My name is Robo Dog.");
        } 
        else if (cmd == 'H') {
            Serial.println("[SAM] Speaking: Status Response");
            sam->Say(out, "I am doing great today!");
        }
        else if (cmd == 'T') {
            Serial.println("[SAM] Speaking: Time Response");
            sam->Say(out, "I do not have a clock yet.");
        } else {
            Serial.print("[SAM] Unknown command received: ");
            Serial.println(cmd);
        }
        
        Serial.println("[DONE] Waiting for next command...");
    }
}