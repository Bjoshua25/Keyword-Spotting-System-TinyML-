#include <driver/i2s.h>

// I2S Configuration
#define I2S_WS 33
#define I2S_SD 32
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0

// Audio parameters for TinyML compatibility
#define SAMPLE_RATE 16000 // 16kHz
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_32BIT // Reading 32-bit slot

// Buffer settings
#define BUFFER_SIZE 512
int32_t sBuffer[BUFFER_SIZE]; // 32-bit buffer

void setup() {
  Serial.begin(115200);
  
  // 1. Define I2S Driver configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // ESP32 is Master, Receiving data
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE, // INMP441 uses 24-bit in 32-bit slot
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // We only care about the grounded L/R pin
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level
    .dma_buf_count = 8,  // Number of DMA buffers
    .dma_buf_len = BUFFER_SIZE, // Size of each DMA buffer
    .use_apll = false, // Not using Audio PLL
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  // 2. Define I2S Pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE, // Not transmitting
    .data_in_num = I2S_SD
  };

  // 3. Install and start the I2S driver
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);
  
  Serial.println("INMP441 Test Started. Open Serial Plotter.");
  delay(1000);
}

void loop() {
  size_t bytesRead = 0;
  
  // 4. Read raw data from DMA
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, BUFFER_SIZE * 4, &bytesRead, portMAX_DELAY);
  
  if (result == ESP_OK) {
    int samplesRead = bytesRead / 4; // Each 32-bit sample is 4 bytes
    
    for (int i = 0; i < samplesRead; i++) {
      // The INMP441 data is 24-bit, but it's justified within the 32-bit slot.
      // Often standard library examples require a shift to align correctly.
      // If the plot looks too quiet, try: int32_t rawSample = sBuffer[i] >> 8;
      
      int32_t rawSample = sBuffer[i]; // Try raw first

      // Print the raw sample to visualze in Serial Plotter
      Serial.println(rawSample);
    }
  }
}