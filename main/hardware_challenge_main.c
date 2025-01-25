// main/hardware_challenge_main.c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "hardware_challenges.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static const char *TAG = "hardware_main";

// Button GPIOs for challenge control
#define BUTTON_NEXT_CHALLENGE GPIO_NUM_39
#define BUTTON_START_STOP     GPIO_NUM_34

// LED indicators for challenge status
#define LED_RUNNING          GPIO_NUM_2
#define LED_VULNERABILITY    GPIO_NUM_4

// Current challenge tracking
static hardware_challenge_type_t current_challenge = HW_CHALLENGE_TIMING_ATTACK;
static bool challenge_running = false;

// Button interrupt handler
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    
    if (gpio_num == BUTTON_NEXT_CHALLENGE) {
        if (!challenge_running) {
            current_challenge = (current_challenge + 1) % 5;
        }
    } else if (gpio_num == BUTTON_START_STOP) {
        challenge_running = !challenge_running;
    }
}

// Initialize GPIO for buttons and LEDs
void init_gpio(void) {
    // Configure button GPIOs
    gpio_config_t btn_config = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_NEXT_CHALLENGE) | (1ULL << BUTTON_START_STOP),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&btn_config);

    // Configure LED GPIOs
    gpio_config_t led_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_RUNNING) | (1ULL << LED_VULNERABILITY),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&led_config);

    // Install GPIO ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_NEXT_CHALLENGE, button_isr_handler, (void*) BUTTON_NEXT_CHALLENGE);
    gpio_isr_handler_add(BUTTON_START_STOP, button_isr_handler, (void*) BUTTON_START_STOP);
}

// Task to control challenge execution
void challenge_control_task(void *pvParameters) {
    bool last_running_state = false;

    while (1) {
        if (challenge_running != last_running_state) {
            if (challenge_running) {
                ESP_LOGI(TAG, "Starting challenge %d", current_challenge);
                start_hardware_challenge(current_challenge);
                gpio_set_level(LED_RUNNING, 1);
            } else {
                ESP_LOGI(TAG, "Stopping current challenge");
                stop_hardware_challenge();
                gpio_set_level(LED_RUNNING, 0);
                gpio_set_level(LED_VULNERABILITY, 0);
            }
            last_running_state = challenge_running;
        }
        
        // Blink vulnerability LED when certain conditions are detected
        if (challenge_running) {
            switch (current_challenge) {
                case HW_CHALLENGE_VOLTAGE_GLITCH:
                    // Blink LED when voltage anomalies are detected
                    if (adc1_get_raw(ADC1_CHANNEL_6) < 1000) {
                        gpio_set_level(LED_VULNERABILITY, 1);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        gpio_set_level(LED_VULNERABILITY, 0);
                    }
                    break;
                    
                case HW_CHALLENGE_TIMING_ATTACK:
                    // Blink LED when timing variations are detected
                    static int64_t last_time = 0;
                    int64_t current_time = esp_timer_get_time();
                    if (current_time - last_time > 1000000) { // 1 second
                        gpio_set_level(LED_VULNERABILITY, 1);
                        vTaskDelay(pdMS_TO_TICKS(50));
                        gpio_set_level(LED_VULNERABILITY, 0);
                        last_time = current_time;
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Print instructions for the current challenge
void print_challenge_instructions(hardware_challenge_type_t challenge) {
    printf("\n=== Hardware Security Challenge Instructions ===\n");
    
    switch (challenge) {
        case HW_CHALLENGE_TIMING_ATTACK:
            printf("Timing Attack Challenge:\n");
            printf("- Observe timing differences in operations\n");
            printf("- Learn about constant-time implementations\n");
            printf("- LED blinks when timing variations detected\n");
            break;
            
        case HW_CHALLENGE_VOLTAGE_GLITCH:
            printf("Voltage Glitch Challenge:\n");
            printf("- Monitor voltage fluctuations\n");
            printf("- Detect potential glitch attacks\n");
            printf("- LED indicates voltage anomalies\n");
            break;
            
        case HW_CHALLENGE_SECURE_BOOT:
            printf("Secure Boot Challenge:\n");
            printf("- Learn about secure boot process\n");
            printf("- Understand signature verification\n");
            printf("- Practice with secure boot configuration\n");
            break;
            
        case HW_CHALLENGE_SIDE_CHANNEL:
            printf("Side-Channel Attack Challenge:\n");
            printf("- Monitor power consumption patterns\n");
            printf("- Understand electromagnetic emissions\n");
            printf("- Learn about countermeasures\n");
            break;
            
        case HW_CHALLENGE_SECURE_STORAGE:
            printf("Secure Storage Challenge:\n");
            printf("- Practice with encrypted storage\n");
            printf("- Understand key protection\n");
            printf("- Learn about secure element usage\n");
            break;
    }
    printf("\nControls:\n");
    printf("- Press NEXT button to cycle through challenges\n");
    printf("- Press START/STOP button to control challenge\n");
    printf("- GREEN LED indicates running challenge\n");
    printf("- RED LED indicates detected vulnerability\n");
    printf("==========================================\n\n");
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize GPIO for buttons and LEDs
    init_gpio();

    // Initialize hardware challenges module
    ESP_ERROR_CHECK(hardware_challenges_init());

    // Create challenge control task
    xTaskCreate(challenge_control_task, "challenge_control", 4096, NULL, 5, NULL);

    // Print initial challenge instructions
    print_challenge_instructions(current_challenge);

    ESP_LOGI(TAG, "Hardware Security Training Platform Started");
    ESP_LOGI(TAG, "Current Challenge: Timing Attack");
    ESP_LOGI(TAG, "Press START button to begin the challenge");

    // Main application loop
    while (1) {
        // Monitor for challenge changes
        static hardware_challenge_type_t last_challenge = -1;
        if (current_challenge != last_challenge) {
            print_challenge_instructions(current_challenge);
            last_challenge = current_challenge;
        }

        // Add a small delay to prevent tight looping
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}