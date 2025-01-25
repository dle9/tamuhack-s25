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
