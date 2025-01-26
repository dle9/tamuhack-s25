#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"
#include "lvgl.h"

// Color definitions using LVGL color format
#define COLOR_BLACK       lv_color_hex(0x000000)
#define COLOR_WHITE       lv_color_hex(0xFFFFFF)
#define COLOR_RED         lv_color_hex(0xFF0000)
#define COLOR_GREEN       lv_color_hex(0x00FF00)
#define COLOR_BLUE        lv_color_hex(0x0000FF)
#define COLOR_YELLOW      lv_color_hex(0xFFFF00)
#define COLOR_CYAN        lv_color_hex(0x00FFFF)
#define COLOR_MAGENTA     lv_color_hex(0xFF00FF)
#define COLOR_GRAY        lv_color_hex(0x808080)

// Display configuration structure
typedef struct {
    uint16_t width;          // Display width
    uint16_t height;         // Display height
    spi_host_device_t spi_host;
    int8_t miso_pin;
    int8_t mosi_pin;
    int8_t sclk_pin;
    int8_t cs_pin;
    int8_t dc_pin;
    int8_t rst_pin;
    int8_t backlight_pin;
    uint32_t clock_speed;
} display_config_t;

// Menu item structure
typedef struct {
    const char *name;
    void (*callback)(void);
} menu_item_t;

// Basic display initialization and control
esp_err_t display_init(display_config_t *config);
void display_set_backlight(bool on);

// UI and drawing functions
void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected);
void display_show_alert(const char *message);
void display_clear(void);

// Optional advanced functions
void display_set_rotation(uint8_t rotation);

// Utility functions
uint16_t display_get_width(void);
uint16_t display_get_height(void);