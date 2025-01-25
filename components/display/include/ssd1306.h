// components/display/include/ssd1306.h
#pragma once

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// SSD1306 display handle
typedef struct ssd1306_dev_t* ssd1306_handle_t;

// Create and initialize SSD1306 device
ssd1306_handle_t ssd1306_create(i2c_port_t i2c_port, uint8_t i2c_addr);

// Display control functions
esp_err_t ssd1306_clear_screen(ssd1306_handle_t dev, uint8_t fill_data);
esp_err_t ssd1306_draw_string(ssd1306_handle_t dev, uint8_t x, uint8_t y, 
                             const char* text, uint8_t font_size, uint8_t color);
esp_err_t ssd1306_refresh_gram(ssd1306_handle_t dev);
esp_err_t ssd1306_display_on(ssd1306_handle_t dev, bool on);

// Drawing primitives
esp_err_t ssd1306_draw_pixel(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t color);
esp_err_t ssd1306_fill_rectangle(ssd1306_handle_t dev, uint8_t x, uint8_t y,
                                uint8_t w, uint8_t h, uint8_t color);
esp_err_t ssd1306_draw_rectangle(ssd1306_handle_t dev, uint8_t x, uint8_t y,
                                uint8_t w, uint8_t h, uint8_t color);

#ifdef __cplusplus
}
#endif