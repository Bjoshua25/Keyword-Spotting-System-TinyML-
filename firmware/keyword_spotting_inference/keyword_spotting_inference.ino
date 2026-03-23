#include <WitAITTS.h>
#include <Voice_Keyword_Spoting_Model_inferencing.h> //
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <esp_wifi.h>

// ==================== 1. FORWARD DECLARATIONS ====================
static void audio_inference_callback(uint32_t n_bytes);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static int i2s_init(uint32_t sampling_rate);
static bool microphone_inference_start(uint32_t n_samples);
static void capture_samples(void* arg);
static void microphone_inference_end(void);

// ==================== 2. CONFIGURATION ====================
const char* WIFI_SSID     = "Godwin"; //
const char* WIFI_PASSWORD = "iiiiiiii"; //
const char* WIT_TOKEN     = "5UMB2NTKO7IAFFESKLO5FHFSWOIKA5HL"; //

#define I2S_PORT I2S_NUM_1 
WitAITTS tts(26, 25, 22); // BCLK, LRC, DIN

typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool record_status = true;

// ==================== 3. SETUP & LOOP ====================
void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Disable WiFi power save to ensure I2S stability
    esp_wifi_set_ps(WIFI_PS_NONE); 

    Serial.println("\n--- Robo-Dog Booting ---");
    Serial.printf("Free Heap Before TTS: %d\n", ESP.getFreeHeap());

    // Initialize TTS first to grab the necessary SSL RAM chunks
    if (tts.begin(WIFI_SSID, WIFI_PASSWORD, WIT_TOKEN)) {
        Serial.println("✓ TTS Ready");
        tts.setVoice("wit$Rebecca");
        tts.setStyle("formal");
        tts.setGain(0.85); // High volume, low distortion
    }

    delay(500); // Settle time

    Serial.printf("Free Heap Before ML: %d\n", ESP.getFreeHeap());
    
    run_classifier_init();
    
    // Initialize Edge Impulse logic
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        Serial.println("!! RAM Exhausted: Inference could not start !!");
        return; 
    }
    
    Serial.printf("Final Free Heap: %d\n", ESP.getFreeHeap());
    Serial.println("✓ Systems Online.");
}

void loop() {
    tts.loop(); // Must run constantly for MP3 streaming

    if (!tts.isBusy()) {
        record_status = true; 
        if (microphone_inference_record()) {
            signal_t signal;
            signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
            signal.get_data = &microphone_audio_signal_get_data;
            ei_impulse_result_t result = {0};

            // Process audio through the ML model
            if (run_classifier_continuous(&signal, &result, false) == EI_IMPULSE_OK) {
                // Mapping: greet [0], hi [1]
                if (result.classification[1].value > 0.8 || result.classification[0].value > 0.8) {
                    record_status = false; 
                    if (result.classification[1].value > 0.8) {
                        Serial.println("Action: Hi Detected");
                        tts.speak("Hello there! I am your robot dog.");
                    } else {
                        Serial.println("Action: Greet Detected");
                        tts.speak("Greetings! How can I help you?");
                    }
                }
            }
        }
    }
    yield(); // Let system background tasks run
}

// ==================== 4. HELPER FUNCTIONS ====================

static void audio_inference_callback(uint32_t n_bytes) {
    int samples_to_process = (int)n_bytes >> 1; 
    for(int i = 0; i < samples_to_process; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];
        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void* arg) {
    const uint32_t i2s_bytes_to_read = (uint32_t)arg;
    size_t bytes_read = 0;
    while (true) {
        if (record_status) {
            i2s_read(I2S_PORT, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, portMAX_DELAY);
            if (bytes_read > 0) {
                audio_inference_callback((uint32_t)bytes_read);
            }
        }
        // Prevents WDT reset by yielding 1ms to the OS
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}

static bool microphone_inference_start(uint32_t n_samples) {
    // Careful allocation to avoid heap fragmentation
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));
    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));
    if (inference.buffers[0] == NULL || inference.buffers[1] == NULL) return false;

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) return false;

    // Pinning to Core 0 leaves Core 1 free for WiFi/Arduino
    // Stack size reduced to 3KB to prevent memory collision
    xTaskCreatePinnedToCore(capture_samples, "CaptureSamples", 1024 * 3, 
                            (void*)sample_buffer_size, 5, NULL, 0);
    return true;
}

static bool microphone_inference_record(void) {
    bool ret = (inference.buf_ready != 0);
    if (ret) inference.buf_ready = 0;
    return ret;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);
    return 0;
}

static int i2s_init(uint32_t sampling_rate) {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = sampling_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = 14, 
        .ws_io_num = 15,  
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = 32, 
    };
    esp_err_t ret = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (ret != ESP_OK) return (int)ret;
    return (int)i2s_set_pin(I2S_PORT, &pin_config);
}

static void microphone_inference_end(void) {
    i2s_driver_uninstall(I2S_PORT);
    free(inference.buffers[0]);
    free(inference.buffers[1]);
}