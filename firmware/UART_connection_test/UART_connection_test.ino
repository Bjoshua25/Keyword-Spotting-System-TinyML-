void setup() {
  // Serial is for the PC Monitor
  Serial.begin(115200);
  // Serial2 is for the connection to ESP 2
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17
  
  Serial.println(">>> ESP 1: UART SENDER READY");
  Serial.println("Type something and press Enter to send to ESP 2...");
}

void loop() {
  // Read from PC Serial Monitor
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    message.trim();
    
    // Send to ESP 2
    Serial2.println(message);
    
    // Local Debug
    Serial.print("Sent to ESP 2: ");
    Serial.println(message);
  }
}