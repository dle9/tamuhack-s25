// main/network_challenge_main.c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "network_challenges.h"
#include "driver/gpio.h"

static const char *TAG = "network_main";

// Button GPIOs for challenge control
#define BUTTON_NEXT_CHALLENGE GPIO_NUM_39
#define BUTTON_START_STOP     GPIO_NUM_34

// Current challenge tracking
static network_challenge_type_t current_challenge = NET_CHALLENGE_BEACON_ANALYSIS;
static bool challenge_running = false;

// Button interrupt handler
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    
    if (gpio_num == BUTTON_NEXT_CHALLENGE) {
        // Cycle through challenges
        if (!challenge_running) {
            current_challenge = (current_challenge + 1) % 5;
        }
    } else if (gpio_num == BUTTON_START_STOP) {
        challenge_running = !challenge_running;
    }
}

void init_buttons(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_NEXT_CHALLENGE) | (1ULL << BUTTON_START_STOP),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_NEXT_CHALLENGE, button_isr_handler, (void*) BUTTON_NEXT_CHALLENGE);
    gpio_isr_handler_add(BUTTON_START_STOP, button_isr_handler, (void*) BUTTON_START_STOP);
}

void challenge_control_task(void *pvParameters) {
    bool last_running_state = false;

    while (1) {
        if (challenge_running != last_running_state) {
            if (challenge_running) {
                ESP_LOGI(TAG, "Starting challenge %d", current_challenge);
                start_network_challenge(current_challenge);
            } else {
                ESP_LOGI(TAG, "Stopping current challenge");
                stop_network_challenge();
            }
            last_running_state = challenge_running;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Initialize WiFi in proper mode for challenges
void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Set WiFi channel to 1 initially
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
}

void print_challenge_instructions(network_challenge_type_t challenge) {
    printf("\n=== Network Security Challenge Instructions ===\n");
    
    switch (challenge) {
        case NET_CHALLENGE_BEACON_ANALYSIS:
            printf("Beacon Frame Analysis Challenge:\n");
            printf("- Learn to identify different types of beacon frames\n");
            printf("- Analyze network security parameters\n");
            printf("- Understand management frame structure\n");
            break;
            
        case NET_CHALLENGE_PACKET_ANALYSIS:
            printf("Packet Analysis Challenge:\n");
            printf("- Identify different types of network traffic\n");
            printf("- Detect suspicious patterns\n");
            printf("- Understand protocol behaviors\n");
            break;
            
        case NET_CHALLENGE_PROTOCOL_SECURITY:
            printf("Protocol Security Challenge:\n");
            printf("- Learn different security protocols\n");
            printf("- Understand encryption methods\n");
            printf("- Identify protocol weaknesses\n");
            break;
            
        case NET_CHALLENGE_DEAUTH_DETECTION:
            printf("Deauthentication Detection Challenge:\n");
            printf("- Identify deauthentication frames\n");
            printf("- Understand attack patterns\n");
            printf("- Learn protection mechanisms\n");
            break;
            
        case NET_CHALLENGE_EVIL_TWIN:
            printf("Evil Twin Detection Challenge:\n");
            printf("- Identify rogue access points\n");
            printf("- Compare network characteristics\n");
            printf("- Learn prevention techniques\n");
            break;
    }
    printf("\nControls:\n");
    printf("- Press NEXT button to cycle through challenges\n");
    printf("- Press START/STOP button to control challenge\n");
    printf("==========================================\n\n");
}

void app_main(void)
{
    // Initialize NVS (Non-Volatile Storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi in the appropriate mode for our challenges
    wifi_init();

    // Initialize the buttons for user interaction
    init_buttons();

    // Initialize the network challenges module
    ESP_ERROR_CHECK(network_challenges_init());

    // Create the challenge control task to manage challenge states
    xTaskCreate(challenge_control_task, "challenge_control", 4096, NULL, 5, NULL);

    // Print initial challenge instructions
    print_challenge_instructions(current_challenge);

    ESP_LOGI(TAG, "Network Security Training Platform Started");
    ESP_LOGI(TAG, "Current Challenge: Beacon Frame Analysis");
    ESP_LOGI(TAG, "Press START button to begin the challenge");

    // Main application loop
    while (1) {
        // Monitor for challenge changes
        static network_challenge_type_t last_challenge = -1;
        if (current_challenge != last_challenge) {
            print_challenge_instructions(current_challenge);
            last_challenge = current_challenge;
        }

        // Add a small delay to prevent tight looping
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}