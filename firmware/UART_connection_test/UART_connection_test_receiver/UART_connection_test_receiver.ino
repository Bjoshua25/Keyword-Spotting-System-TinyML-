void setup() {
  // Serial is for the PC Monitor
  Serial.begin(115200);
  // Serial2 is for the connection from ESP 1
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17
  
  Serial.println(">>> ESP 2: UART RECEIVER READY");
  Serial.println("Waiting for data from ESP 1...");
}

void loop() {
  // Check if ESP 1 sent anything
  if (Serial2.available()) {
    String received = Serial2.readStringUntil('\n');
    received.trim();
    
    // Print to PC Serial Monitor
    Serial.print("MESSAGE RECEIVED: ");
    Serial.println(received);
  }
}