// components/display/include/display.h
#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"
#include "esp_lcd_types.h"

// Color definitions
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_GRAY        0x8410

// Display configuration structure
typedef struct {
    uint16_t width;          // Changed to uint16_t for larger displays
    uint16_t height;
    spi_host_device_t spi_host;
    int8_t miso_pin;
    int8_t mosi_pin;
    int8_t sclk_pin;
    int8_t cs_pin;
    int8_t dc_pin;
    int8_t rst_pin;
    int8_t backlight_pin;
    uint32_t clock_speed;    // Added for SPI clock configuration
} display_config_t;

// Menu item structure
typedef struct {
    const char *name;
    void (*callback)(void);
} menu_item_t;

// Font size enumeration
typedef enum {
    FONT_SMALL = 1,
    FONT_MEDIUM = 2,
    FONT_LARGE = 3
} font_size_t;

// Basic shape and drawing functions
esp_err_t display_init(display_config_t *config);
void display_set_rotation(uint8_t rotation);
void display_set_backlight(bool on);

// Drawing functions
void display_fill_screen(uint16_t color);
void display_draw_pixel(int16_t x, int16_t y, uint16_t color);
void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void display_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void display_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

// Text functions
void display_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, font_size_t size);
void display_draw_text(int16_t x, int16_t y, const char *text, uint16_t color, uint16_t bg, font_size_t size);

// High-level UI functions
void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected);
void display_show_progress(const char *message, uint8_t progress);
void display_show_alert(const char *message);
void display_clear(void);

// Added helper functions
void display_draw_button(int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint16_t color);
void display_draw_header(const char *text, uint16_t color);
void display_draw_footer(const char *text, uint16_t color);

// Display dimensions getters
uint16_t display_get_width(void);
uint16_t display_get_height(void);