// components/display/include/display.h
#pragma once

#include "esp_err.h"
#include "driver/i2c.h"

// Display configuration structure
typedef struct {
    uint8_t width;
    uint8_t height;
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    int sda_pin;
    int scl_pin;
} display_config_t;

// Menu item structure
typedef struct {
    const char *name;
    void (*callback)(void);
} menu_item_t;

esp_err_t display_init(display_config_t *config);
void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected);
void display_show_progress(const char *message, uint8_t progress);
void display_show_alert(const char *message);
void display_clear(void);

// components/display/display.c
#include "display.h"
#include "ssd1306.h"
#include "esp_log.h"

static const char *TAG = "display";
static ssd1306_handle_t ssd1306_dev = NULL;

// SSD1306 commands
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3

esp_err_t display_init(display_config_t *config) {
    ESP_LOGI(TAG, "Initializing display");
    
    // Initialize I2C
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    
    ESP_ERROR_CHECK(i2c_param_config(config->i2c_port, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(config->i2c_port, I2C_MODE_MASTER, 0, 0, 0));

    // Initialize SSD1306
    ssd1306_dev = ssd1306_create(config->i2c_port, config->i2c_addr);
    if (!ssd1306_dev) {
        ESP_LOGE(TAG, "SSD1306 device creation failed");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(ssd1306_refresh_gram(ssd1306_dev));
    ESP_ERROR_CHECK(ssd1306_clear_screen(ssd1306_dev, 0x00));
    ESP_ERROR_CHECK(ssd1306_display_on(ssd1306_dev, true));

    return ESP_OK;
}

void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected) {
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    
    // Draw title
    ssd1306_draw_string(ssd1306_dev, 0, 0, "ESP32 Security Trainer", 16, 1);
    
    // Draw menu items
    for (size_t i = 0; i < num_items; i++) {
        char buffer[32];
        if (i == selected) {
            snprintf(buffer, sizeof(buffer), "> %s", items[i].name);
        } else {
            snprintf(buffer, sizeof(buffer), "  %s", items[i].name);
        }
        ssd1306_draw_string(ssd1306_dev, 0, (i + 2) * 16, buffer, 16, 1);
    }
    
    ssd1306_refresh_gram(ssd1306_dev);
}

void display_show_progress(const char *message, uint8_t progress) {
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    
    // Draw message
    ssd1306_draw_string(ssd1306_dev, 0, 0, message, 16, 1);
    
    // Draw progress bar
    uint8_t bar_width = (progress * 120) / 100;
    ssd1306_fill_rectangle(ssd1306_dev, 4, 32, bar_width, 8, 1);
    ssd1306_draw_rectangle(ssd1306_dev, 2, 30, 124, 12, 1);
    
    ssd1306_refresh_gram(ssd1306_dev);
}

void display_show_alert(const char *message) {
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    ssd1306_draw_string(ssd1306_dev, 0, 0, "ALERT:", 16, 1);
    ssd1306_draw_string(ssd1306_dev, 0, 16, message, 16, 1);
    ssd1306_refresh_gram(ssd1306_dev);
}

void display_clear(void) {
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    ssd1306_refresh_gram(ssd1306_dev);
}