// main/main.c
#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "display.h"
#include "network_module.h"
#include "web_module.h"
#include "hardware_module.h"

static const char *TAG = "main";

// Button GPIO configurations
#define BUTTON_UP GPIO_NUM_35
#define BUTTON_DOWN GPIO_NUM_34
#define BUTTON_SELECT GPIO_NUM_39

// Display configuration
#define DISPLAY_SDA GPIO_NUM_21
#define DISPLAY_SCL GPIO_NUM_22
#define DISPLAY_ADDRESS 0x3C

// Event queue for button presses
static QueueHandle_t button_evt_queue = NULL;

// Menu items and callbacks
void network_training_cb(void);
void web_training_cb(void);
void hardware_training_cb(void);

static menu_item_t menu_items[] = {
    {"Network Security", network_training_cb},
    {"Web Security", web_training_cb},
    {"Hardware Security", hardware_training_cb}
};

static size_t current_menu_item = 0;
static const size_t num_menu_items = sizeof(menu_items) / sizeof(menu_items[0]);

// Button interrupt handler
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

// Button handling task
static void button_task(void* arg) {
    uint32_t gpio_num;
    for(;;) {
        if(xQueueReceive(button_evt_queue, &gpio_num, portMAX_DELAY)) {
            // Debounce
            vTaskDelay(pdMS_TO_TICKS(50));
            
            if(gpio_num == BUTTON_UP) {
                if(current_menu_item > 0) current_menu_item--;
            } else if(gpio_num == BUTTON_DOWN) {
                if(current_menu_item < num_menu_items - 1) current_menu_item++;
            } else if(gpio_num == BUTTON_SELECT) {
                if(menu_items[current_menu_item].callback) {
                    menu_items[current_menu_item].callback();
                }
            }
            
            display_show_menu(menu_items, num_menu_items, current_menu_item);
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32 Security Trainer");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize display
    display_config_t display_config = {
        .width = 128,
        .height = 64,
        .i2c_port = I2C_NUM_0,
        .i2c_addr = DISPLAY_ADDRESS,
        .sda_pin = DISPLAY_SDA,
        .scl_pin = DISPLAY_SCL
    };
    ESP_ERROR_CHECK(display_init(&display_config));

    // Initialize button handling
    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    // Configure button GPIOs
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_UP) | (1ULL << BUTTON_DOWN) | (1ULL << BUTTON_SELECT),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    // Install GPIO ISR service
    gpio_install_isr_service(0);
    
    // Add ISR handlers for buttons
    gpio_isr_handler_add(BUTTON_UP, button_isr_handler, (void*) BUTTON_UP);
    gpio_isr_handler_add(BUTTON_DOWN, button_isr_handler, (void*) BUTTON_DOWN);
    gpio_isr_handler_add(BUTTON_SELECT, button_isr_handler, (void*) BUTTON_SELECT);

    // Create button handling task
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    // Initialize training modules
    network_module_init();
    web_module_init();
    hardware_module_init();

    // Show initial menu
    display_show_menu(menu_items, num_menu_items, current_menu_item);
}
