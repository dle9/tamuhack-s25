// main/bluetooth_challenge_main.c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bluetooth_challenges.h"
#include "driver/gpio.h"

static const char *TAG = "bluetooth_main";

// Button GPIOs for challenge control
#define BUTTON_NEXT_CHALLENGE GPIO_NUM_39
#define BUTTON_START_STOP     GPIO_NUM_34

// LED indicators for challenge status
#define LED_RUNNING          GPIO_NUM_2
#define LED_DETECTION        GPIO_NUM_4

// Current challenge tracking
static bluetooth_challenge_type_t current_challenge = BT_CHALLENGE_SCANNING;
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
        .pin_bit_mask = (1ULL << LED_RUNNING) | (1ULL << LED_DETECTION),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };
    gpio_config(&led_config);

    // Install GPIO ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_NEXT_CHALLENGE, button_isr_handler, (void*) BUTTON_NEXT_CHALLENGE);
    gpio_isr_handler_add(BUTTON_START_STOP, button_isr_handler, (void*) BUTTON_START_STOP);
}

// Task to control challenge execution and monitor status
void challenge_control_task(void *pvParameters) {
    bool last_running_state = false;

    while (1) {
        // Handle challenge state changes
        if (challenge_running != last_running_state) {
            if (challenge_running) {
                ESP_LOGI(TAG, "Starting challenge %d", current_challenge);
                start_bluetooth_challenge(current_challenge);
                gpio_set_level(LED_RUNNING, 1);
            } else {
                ESP_LOGI(TAG, "Stopping current challenge");
                stop_bluetooth_challenge();
                gpio_set_level(LED_RUNNING, 0);
                gpio_set_level(LED_DETECTION, 0);
            }
            last_running_state = challenge_running;
        }
        
        // Monitor for security events and control LED indicators
        if (challenge_running) {
            switch (current_challenge) {
                case BT_CHALLENGE_SCANNING:
                    // Blink detection LED when new devices are found
                    static uint32_t last_device_count = 0;
                    uint32_t current_count = 0; // You would implement a way to get this
                    if (current_count > last_device_count) {
                        gpio_set_level(LED_DETECTION, 1);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        gpio_set_level(LED_DETECTION, 0);
                        last_device_count = current_count;
                    }
                    break;
                    
                case BT_CHALLENGE_PAIRING:
                    // LED indicates pairing activity
                    // LED patterns could indicate security level of pairing
                    break;
                    
                case BT_CHALLENGE_MAN_IN_MIDDLE:
                    // LED indicates potential MITM detection
                    break;
                    
                case BT_CHALLENGE_SNIFFING:
                    // LED indicates when interesting packets are captured
                    break;
                    
                case BT_CHALLENGE_SPOOFING:
                    // LED indicates detection of spoofed devices
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Print instructions for the current challenge
void print_challenge_instructions(bluetooth_challenge_type_t challenge) {
    printf("\n=== Bluetooth Security Challenge Instructions ===\n");
    
    switch (challenge) {
        case BT_CHALLENGE_SCANNING:
            printf("BLE Scanning Challenge:\n");
            printf("- Learn to identify different types of BLE devices\n");
            printf("- Analyze advertisement data\n");
            printf("- Understand device discovery process\n");
            break;
            
        case BT_CHALLENGE_PAIRING:
            printf("Pairing Security Challenge:\n");
            printf("- Understand different pairing methods\n");
            printf("- Learn about authentication levels\n");
            printf("- Practice secure pairing procedures\n");
            break;
            
        case BT_CHALLENGE_MAN_IN_MIDDLE:
            printf("Man-in-the-Middle Detection Challenge:\n");
            printf("- Learn to identify MITM attempts\n");
            printf("- Understand session security\n");
            printf("- Practice secure connection verification\n");
            break;
            
        case BT_CHALLENGE_SNIFFING:
            printf("Packet Sniffing Analysis Challenge:\n");
            printf("- Capture and analyze BLE packets\n");
            printf("- Identify sensitive information\n");
            printf("- Learn about packet encryption\n");
            break;
            
        case BT_CHALLENGE_SPOOFING:
            printf("Device Spoofing Detection Challenge:\n");
            printf("- Learn to identify spoofed devices\n");
            printf("- Understand device authentication\n");
            printf("- Practice device validation techniques\n");
            break;
    }
    
    printf("\nControls:\n");
    printf("- Press NEXT button to cycle through challenges\n");
    printf("- Press START/STOP button to control challenge\n");
    printf("- GREEN LED indicates running challenge\n");
    printf("- RED LED indicates security events\n");
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

    // Initialize Bluetooth challenges module
    ESP_ERROR_CHECK(bluetooth_challenges_init());

    // Create challenge control task
    xTaskCreate(challenge_control_task, "challenge_control", 4096, NULL, 5, NULL);

    // Print initial challenge instructions
    print_challenge_instructions(current_challenge);

    ESP_LOGI(TAG, "Bluetooth Security Training Platform Started");
    ESP_LOGI(TAG, "Current Challenge: BLE Scanning");
    ESP_LOGI(TAG, "Press START button to begin the challenge");

    // Main application loop
    while (1) {
        // Monitor for challenge changes
        static bluetooth_challenge_type_t last_challenge = -1;
        if (current_challenge != last_challenge) {
            print_challenge_instructions(current_challenge);
            last_challenge = current_challenge;
        }

        // Add a small delay to prevent tight looping
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}