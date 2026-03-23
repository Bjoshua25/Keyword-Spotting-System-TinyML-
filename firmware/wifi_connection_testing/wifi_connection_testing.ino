// ESP32 Wi-Fi Hotspot Test with LED Indication

#include <WiFi.h>

// Your Hotspot Credentials
const char* ssid = "GodWin";
const char* password = "iiiiiiii";

// LED Indication Pin
#define STATUS_LED_PIN 2 // Many dev boards have an onboard LED here

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Initialize LED Pin
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW); // Start with LED OFF

  Serial.println("");
  Serial.println("======================================");
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // 2. Monitor status & BLINK LED while connecting
  Serial.print("Connecting");
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED) {
    // Rapid Blink pattern (100ms ON, 400ms OFF = 2Hz)
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(400); 
    
    Serial.print(".");
    
    // Timeout check (15 seconds)
    if (millis() - startAttemptTime > 15000) {
       Serial.println("");
       Serial.println("Connection Failed: Timeout.");
       // Rapid strobe blink to indicate error
       for(int i=0; i<10; i++) {
         digitalWrite(STATUS_LED_PIN, HIGH); delay(50);
         digitalWrite(STATUS_LED_PIN, LOW); delay(50);
       }
       while(1); // Halt here
    }
  }

  // 3. Connection Successful -> SOLID ON LED
  digitalWrite(STATUS_LED_PIN, HIGH); 
  
  Serial.println("");
  Serial.println("======================================");
  Serial.println("SUCCESS! Connected to Internet.");
  Serial.print("LED on GPIO ");
  Serial.print(STATUS_LED_PIN);
  Serial.println(" is now Solid ON.");
  
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("======================================");
}

void loop() {
  // Main connection maintenance check
  if (WiFi.status() != WL_CONNECTED) {
    // If connection lost, turn LED OFF
    digitalWrite(STATUS_LED_PIN, LOW);
    Serial.println("Warning: WiFi Connection Lost! LED OFF.");
    
    // Attempt reconnection (optional addition for final project)
    // WiFi.begin(ssid, password); 
  } else {
    // Ensure LED stays ON if connected
    digitalWrite(STATUS_LED_PIN, HIGH);
  }
  delay(5000); 
}