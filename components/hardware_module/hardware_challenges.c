// components/hardware_module/hardware_challenges.c
#include "hardware_challenges.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/temp_sensor.h"
#include "esp_flash_encrypt.h"
#include <string.h>

static const char *TAG = "hardware_challenges";

// Handle for the challenge task
static TaskHandle_t challenge_task_handle = NULL;

// Current active challenge
static hardware_challenge_type_t active_challenge = -1;

// ADC calibration for voltage monitoring
static esp_adc_cal_characteristics_t adc_chars;

// Simulated secure data for challenges
static const uint8_t secure_data[] = {
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

// Function to simulate a timing-vulnerable password check
static bool timing_vulnerable_check(const char* input, const char* password) {
    size_t len = strlen(password);
    for (size_t i = 0; i < len; i++) {
        if (input[i] != password[i]) {
            return false;
        }
        // Intentionally vulnerable delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return true;
}

// Task to demonstrate timing attack vulnerabilities
static void timing_attack_task(void *pvParameters) {
    const char* secret_password = "SecretPass123";
    char test_input[] = "aaaaaaaaaaaaa";
    
    ESP_LOGI(TAG, "Starting Timing Attack Challenge");
    ESP_LOGI(TAG, "Try to determine the password by measuring response times");
    
    while (active_challenge == HW_CHALLENGE_TIMING_ATTACK) {
        // Measure time taken for password check
        int64_t start = esp_timer_get_time();
        bool result = timing_vulnerable_check(test_input, secret_password);
        int64_t end = esp_timer_get_time();
        
        ESP_LOGI(TAG, "Check result: %d, Time taken: %lld us", result, end - start);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    vTaskDelete(NULL);
}

// Task to demonstrate voltage glitch detection
static void voltage_glitch_task(void *pvParameters) {
    // Configure ADC for voltage monitoring
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
    ESP_LOGI(TAG, "Starting Voltage Glitch Detection Challenge");
    
    while (active_challenge == HW_CHALLENGE_VOLTAGE_GLITCH) {
        // Read voltage and check for anomalies
        uint32_t voltage = 0;
        for (int i = 0; i < 10; i++) {
            voltage += adc1_get_raw(ADC1_CHANNEL_6);
        }
        voltage /= 10;
        
        uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(voltage, &adc_chars);
        
        // Check for suspicious voltage variations
        if (voltage_mv < 2700 || voltage_mv > 3600) {
            ESP_LOGW(TAG, "Potential voltage glitch detected! Voltage: %d mV", voltage_mv);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    vTaskDelete(NULL);
}

// Task to demonstrate secure boot concepts
static void secure_boot_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting Secure Boot Challenge");
    
    // Check if secure boot is enabled
    bool secure_boot_enabled = esp_secure_boot_enabled();
    ESP_LOGI(TAG, "Secure Boot Status: %s", secure_boot_enabled ? "Enabled" : "Disabled");
    
    while (active_challenge == HW_CHALLENGE_SECURE_BOOT) {
        // Demonstrate signature verification process
        ESP_LOGI(TAG, "Simulating secure boot process:");
        ESP_LOGI(TAG, "1. Verify bootloader signature");
        ESP_LOGI(TAG, "2. Check flash encryption status");
        ESP_LOGI(TAG, "3. Validate application signature");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
    vTaskDelete(NULL);
}

// Task to demonstrate side-channel attack concepts
static void side_channel_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting Side-Channel Attack Challenge");
    
    // Configure temperature sensor
    temp_sensor_config_t temp_config = TEMP_SENSOR_CONFIG_DEFAULT(10, 50);
    ESP_ERROR_CHECK(temp_sensor_set_config(temp_config));
    ESP_ERROR_CHECK(temp_sensor_start());
    
    while (active_challenge == HW_CHALLENGE_SIDE_CHANNEL) {
        // Monitor power consumption patterns
        float temperature;
        ESP_ERROR_CHECK(temp_sensor_read_celsius(&temperature));
        
        // Simulate different cryptographic operations
        for (int i = 0; i < sizeof(secure_data); i++) {
            // Perform "sensitive" operations
            volatile uint8_t temp = secure_data[i];
            // Create observable power pattern
            for (int j = 0; j < temp; j++) {
                asm volatile("nop");
            }
        }
        
        ESP_LOGI(TAG, "Temperature during operation: %.2f°C", temperature);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    temp_sensor_stop();
    vTaskDelete(NULL);
}

// Task to demonstrate secure storage concepts
static void secure_storage_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting Secure Storage Challenge");
    
    // Check flash encryption status
    bool flash_encryption_enabled = esp_flash_encryption_enabled();
    ESP_LOGI(TAG, "Flash Encryption Status: %s", 
             flash_encryption_enabled ? "Enabled" : "Disabled");
    
    while (active_challenge == HW_CHALLENGE_SECURE_STORAGE) {
        // Demonstrate secure storage operations
        ESP_LOGI(TAG, "Secure Storage Operations:");
        ESP_LOGI(TAG, "1. Using hardware-encrypted flash");
        ESP_LOGI(TAG, "2. Implementing secure key storage");
        ESP_LOGI(TAG, "3. Protected storage regions");
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    
    vTaskDelete(NULL);
}

esp_err_t hardware_challenges_init(void) {
    ESP_LOGI(TAG, "Initializing hardware security challenges");
    return ESP_OK;
}

esp_err_t start_hardware_challenge(hardware_challenge_type_t type) {
    if (active_challenge != -1) {
        ESP_LOGE(TAG, "Challenge already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    active_challenge = type;
    
    switch (type) {
        case HW_CHALLENGE_TIMING_ATTACK:
            xTaskCreate(timing_attack_task, "timing_attack", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case HW_CHALLENGE_VOLTAGE_GLITCH:
            xTaskCreate(voltage_glitch_task, "voltage_glitch", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case HW_CHALLENGE_SECURE_BOOT:
            xTaskCreate(secure_boot_task, "secure_boot", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case HW_CHALLENGE_SIDE_CHANNEL:
            xTaskCreate(side_channel_task, "side_channel", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case HW_CHALLENGE_SECURE_STORAGE:
            xTaskCreate(secure_storage_task, "secure_storage", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        default:
            ESP_LOGE(TAG, "Unknown challenge type");
            return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Started hardware challenge type %d", type);
    return ESP_OK;
}

esp_err_t stop_hardware_challenge(void) {
    if (active_challenge == -1) {
        return ESP_OK;
    }
    
    active_challenge = -1;
    vTaskDelay(pdMS_TO_TICKS(100));  // Give time for task cleanup
    
    if (challenge_task_handle != NULL) {
        vTaskDelete(challenge_task_handle);
        challenge_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "Stopped hardware challenge");
    return ESP_OK;
}