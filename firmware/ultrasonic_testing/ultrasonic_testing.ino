// HC-SR04 Standalone Test for ESP32

// Pin Definitions
#define TRIG_PIN 5  // Connected to Trig (Safe 3.3V Out)
#define ECHO_PIN 18 // Connected to Echo (VIA DIVIDER)

// Speed of sound at sea level (approx 20C) in cm/us
#define SOUND_SPEED 0.0343 

void setup() {
  Serial.begin(115200); // Initialize serial communication
  
  // Define pin modes
  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT);  // Sets the echoPin as an Input
  
  // Ensure Trig starts LOW
  digitalWrite(TRIG_PIN, LOW);
  
  Serial.println("HC-SR04 Standalone Test Started.");
  delay(1000);
}

void loop() {
  long duration;
  float distanceCm;

  // 1. Generate the Trigger Pulse
  // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 2. Measure the Echo Pulse Duration
  // Reads the echoPin, returns the sound wave travel time in microseconds
  // Timeout set to 26ms (max range approx 4.5m * 2 / 343m/s)
  duration = pulseIn(ECHO_PIN, HIGH, 26000); 

  // 3. Calculate Distance
  if (duration == 0) {
    // If no pulse received within timeout
    Serial.println("Error: No pulse received (Out of Range or Wiring Issue)");
  } else {
    // Calculate the distance: (Time * Speed) / 2 (Round trip)
    distanceCm = duration * SOUND_SPEED / 2;
    
    // Print the distance to Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
  }

  // Wait 100ms before next measurement to avoid ultrasonic interference
  delay(5000); 
}