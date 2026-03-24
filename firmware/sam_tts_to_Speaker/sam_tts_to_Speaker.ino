#include <Arduino.h>
#include "ESP8266SAM.h"
#include "AudioOutputI2S.h"

// I2S Pins for MAX98357A
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 22

AudioOutputI2S *out;
ESP8266SAM *sam;

void setup() {
    Serial.begin(115200);
    
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    sam = new ESP8266SAM();

    Serial.println("\n--- SAM Interactive Voice Console ---");
    Serial.println("Format: [Pitch,Speed]Text");
    Serial.println("Example: [64,72]I am a happy robot dog");
    Serial.println("Example: [30,50]I am a grumpy robot dog");
    Serial.println("--------------------------------------");
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.length() > 0) {
            int pitch = 20; // Default
            int speed = 90; // Default
            String message = input;

            // Simple parser for [Pitch,Speed]
            if (input.startsWith("[")) {
                int commaIndex = input.indexOf(',');
                int closeBracketIndex = input.indexOf(']');
                
                if (commaIndex > 0 && closeBracketIndex > commaIndex) {
                    pitch = input.substring(1, commaIndex).toInt();
                    speed = input.substring(commaIndex + 1, closeBracketIndex).toInt();
                    message = input.substring(closeBracketIndex + 1);
                }
            }

            // Apply parameters
            sam->SetPitch(pitch);
            sam->SetSpeed(speed);
            
            Serial.printf("Speaking (P:%d, S:%d): %s\n", pitch, speed, message.c_str());
            sam->Say(out, message.c_str());
        }
    }
}




// Try these specific commands in the Serial Monitor:

// Excited Puppy: [110,100]Oh boy! Oh boy! A ball!

// Big Guard Dog: [30,60]Stay away from my house!

// Broken Robot: [50,30]Battery level is low... goodbye.