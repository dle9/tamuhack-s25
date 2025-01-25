// components/hardware_module/hardware_challenges.c
#include "hardware_challenges.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_secure_boot.h"

static const char *TAG = "hardware_challenges";

// Current active challenge
static hardware_challenge_type_t active_challenge = -1;
static TaskHandle_t challenge_task_handle = NULL;

// ADC configuration
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;

// Hardware security monitoring task
void voltage_glitch_task(void *pvParameters) {
    // Configure ADC for voltage monitoring
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));

    while (active_challenge == HW_CHALLENGE_VOLTAGE_GLITCH) {
        int adc_raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw));
        
        // Check for voltage anomalies
        if (adc_raw < 1000 || adc_raw > 3000) {
            ESP_LOGW(TAG, "Voltage glitch detected! Raw ADC: %d", adc_raw);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    vTaskDelete(NULL);
}

void secure_boot_task(void *pvParameters) {
    ESP_LOGI(TAG, "Demonstrating Secure Boot concepts");

    while (active_challenge == HW_CHALLENGE_SECURE_BOOT) {
        // Check secure boot status
        if (esp_secure_boot_enabled()) {
            ESP_LOGI(TAG, "Secure Boot is enabled");
        } else {
            ESP_LOGI(TAG, "Secure Boot is not enabled");
        }

        // Demonstrate flash encryption status
        if (esp_flash_encryption_enabled()) {
            ESP_LOGI(TAG, "Flash encryption is enabled");
        } else {
            ESP_LOGI(TAG, "Flash encryption is not enabled");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    vTaskDelete(NULL);
}

void timing_attack_task(void *pvParameters) {
    // Simulate a timing vulnerability
    const char* secret = "SecretPassword123";
    char test_input[] = "TestPassword123456";
    
    while (active_challenge == HW_CHALLENGE_TIMING_ATTACK) {
        int64_t start = esp_timer_get_time();
        
        // Intentionally vulnerable comparison
        bool match = true;
        for (int i = 0; i < strlen(secret); i++) {
            if (test_input[i] != secret[i]) {
                match = false;
                break;
            }
            vTaskDelay(1);  // Artificial delay for demonstration
        }
        
        int64_t end = esp_timer_get_time();
        ESP_LOGI(TAG, "Time taken: %lld µs, Match: %d", end - start, match);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
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
        case HW_CHALLENGE_VOLTAGE_GLITCH:
            xTaskCreate(voltage_glitch_task, "voltage_glitch", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case HW_CHALLENGE_SECURE_BOOT:
            xTaskCreate(secure_boot_task, "secure_boot", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case HW_CHALLENGE_TIMING_ATTACK:
            xTaskCreate(timing_attack_task, "timing_attack", 4096, NULL, 5, &challenge_task_handle);
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