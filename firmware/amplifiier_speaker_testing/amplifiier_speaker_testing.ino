#include <driver/i2s.h>
#include <math.h>

// I2S Configuration Pins
#define I2S_WS 25
#define I2S_BCLK 26
#define I2S_DOUT 22
#define I2S_PORT I2S_NUM_0

// Audio Parameters for the Tone
#define SAMPLE_RATE 44100 // Standard audio sample rate
#define TONE_FREQ_HZ 440 // A4 note (standard tone frequency)
#define AMPLITUDE 10000 // Volume (Max for int16 is 32767)

// Generate one cycle of a sine wave tone
void generateSineWave(int16_t* buffer, int samples, float freq, float sampleRate) {
    for (int i = 0; i < samples; i++) {
        buffer[i] = (int16_t)(AMPLITUDE * sin(2.0 * PI * freq * i / sampleRate));
    }
}

void setup() {
  Serial.begin(115200);

  // 1. Define I2S Driver configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Master, Transmitting
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // The MAX98357A accepts 16-bit PCM
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Output on Left (SD is Mono mode average)
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level
    .dma_buf_count = 4,  // Number of DMA buffers
    .dma_buf_len = 1024, // Size of each DMA buffer
    .use_apll = false, // APLL not required
    .tx_desc_auto_clear = true, // Clear buffer automatically
    .fixed_mclk = 0
  };

  // 2. Define I2S Pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE // Not receiving
  };

  // 3. Install and start the I2S driver
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);

  Serial.println("MAX98357A Tone Test Started. You should hear a tone.");
  delay(1000);
}

void loop() {
  const int bufferSize = 1024;
  int16_t audioBuffer[bufferSize];

  // 4. Generate Sine Wave into Buffer
  generateSineWave(audioBuffer, bufferSize, TONE_FREQ_HZ, SAMPLE_RATE);

  // 5. Write raw data to DMA for transmission
  size_t bytesWritten = 0;
  // bytesWritten will return the number of bytes actually written
  esp_err_t result = i2s_write(I2S_PORT, audioBuffer, bufferSize * 2, &bytesWritten, portMAX_DELAY);

  if (result != ESP_OK) {
    Serial.println("Error writing to I2S.");
  }
}