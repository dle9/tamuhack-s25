// components/display/display.c
#include "display.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "font8x8_basic.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"

// Display dimensions
static uint16_t screen_width;
static uint16_t screen_height;
static spi_device_handle_t spi;
static display_config_t CONFIG;

// Basic font 8x8 for initial implementation
#include "font8x8_basic.h"

// Define MIN macro if not already defined
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// Function declarations
static void lcd_cmd(uint8_t cmd);
static void lcd_data(const uint8_t *data, int len);
static void lcd_init_cmds(void);

// Send command to display
static void lcd_cmd(uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;
    gpio_set_level(CONFIG.dc_pin, 0);
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

// Send data to display
static void lcd_data(const uint8_t *data, int len) {
    if (len == 0) return;
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    t.user = (void*)1;
    gpio_set_level(CONFIG.dc_pin, 1);
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

static void lcd_init_cmds(void) {
    // Software Reset
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_cmd(0xCF);
    {
        uint8_t data[] = {0x00, 0xC1, 0X30};
        lcd_data(data, 3);
    }

    lcd_cmd(0xED);
    {
        uint8_t data[] = {0x64, 0x03, 0X12, 0X81};
        lcd_data(data, 4);
    }

    lcd_cmd(0xE8);
    {
        uint8_t data[] = {0x85, 0x00, 0x78};
        lcd_data(data, 3);
    }

    lcd_cmd(0xCB);
    {
        uint8_t data[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
        lcd_data(data, 5);
    }

    lcd_cmd(0xF7);
    {
        uint8_t data[] = {0x20};
        lcd_data(data, 1);
    }

    lcd_cmd(0xEA);
    {
        uint8_t data[] = {0x00, 0x00};
        lcd_data(data, 2);
    }

    lcd_cmd(0xC0);    // Power Control 1
    {
        uint8_t data[] = {0x23};
        lcd_data(data, 1);
    }

    lcd_cmd(0xC1);    // Power Control 2
    {
        uint8_t data[] = {0x10};
        lcd_data(data, 1);
    }

    lcd_cmd(0xC5);    // VCOM Control 1
    {
        uint8_t data[] = {0x3e, 0x28};
        lcd_data(data, 2);
    }

    lcd_cmd(0xC7);    // VCOM Control 2
    {
        uint8_t data[] = {0x86};
        lcd_data(data, 1);
    }

    lcd_cmd(0x36);    // Memory Access Control
    {
        // Try each of these values one at a time:
        // uint8_t data[] = {0x08};  // Default
        uint8_t data[] = {0x68};  // Rotation 90
        // uint8_t data[] = {0xC8};  // Rotation 180 - try this one first
        //uint8_t data[] = {0xA8};  // Rotation 270
        lcd_data(data, 1);
    }

    lcd_cmd(0x3A);    // Pixel Format Set
    {
        uint8_t data[] = {0x55};
        lcd_data(data, 1);
    }

    lcd_cmd(0xB1);    // Frame Rate Control
    {
        uint8_t data[] = {0x00, 0x18};
        lcd_data(data, 2);
    }

    lcd_cmd(0xB6);    // Display Function Control
    {
        uint8_t data[] = {0x08, 0x82, 0x27};
        lcd_data(data, 3);
    }

    lcd_cmd(0xF2);    // Enable 3G
    {
        uint8_t data[] = {0x00};
        lcd_data(data, 1);
    }

    lcd_cmd(0x26);    // Gamma Set
    {
        uint8_t data[] = {0x01};
        lcd_data(data, 1);
    }

    // Exit Sleep
    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Display on
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(20));
}

// Function to initialize the display
esp_err_t display_init(display_config_t *config) {
    memcpy(&CONFIG, config, sizeof(display_config_t));
    screen_width = config->width;
    screen_height = config->height;

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = config->miso_pin,
        .mosi_io_num = config->mosi_pin,
        .sclk_io_num = config->sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = config->width * config->height * 2 + 8
    };

    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = config->clock_speed,
        .mode = 0,
        .spics_io_num = config->cs_pin,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,  // Add this line
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    // Initialize pins
    gpio_set_direction(config->dc_pin, GPIO_MODE_OUTPUT);
    if (config->rst_pin >= 0) {
        gpio_set_direction(config->rst_pin, GPIO_MODE_OUTPUT);
    }
    gpio_set_direction(config->backlight_pin, GPIO_MODE_OUTPUT);

    // Initialize SPI
    ESP_ERROR_CHECK(spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(config->spi_host, &devcfg, &spi));

    // Reset display if RST pin is connected
    if (config->rst_pin >= 0) {
        gpio_set_level(config->rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(config->rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    lcd_init_cmds();

    // Turn on backlight
    gpio_set_level(config->backlight_pin, 1);

    // Clear screen
    display_fill_screen(COLOR_BLACK);

    return ESP_OK;
}

// Implement other required functions...
void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected) {
    // Clear screen
    display_fill_screen(COLOR_BLACK);
    
    // Draw title
    display_draw_text(10, 10, "ESP32 Security Trainer", COLOR_WHITE, COLOR_BLACK, FONT_MEDIUM);
    
    // Draw menu items
    for (size_t i = 0; i < num_items; i++) {
        uint16_t y = 50 + (i * 30);
        uint16_t color = (i == selected) ? COLOR_GREEN : COLOR_WHITE;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%s%s", (i == selected) ? "> " : "  ", items[i].name);
        display_draw_text(20, y, buffer, color, COLOR_BLACK, FONT_MEDIUM);
    }
}

void display_fill_screen(uint16_t color) {
    uint16_t buffer[32];  // Buffer for faster filling
    for (int i = 0; i < 32; i++) {
        buffer[i] = color;
    }
    
    lcd_cmd(0x2A);    // Column address set
    {
        uint8_t data[] = {0x00, 0x00, (screen_width >> 8) & 0xFF, screen_width & 0xFF};
        lcd_data(data, 4);
    }
    
    lcd_cmd(0x2B);    // Row address set
    {
        uint8_t data[] = {0x00, 0x00, (screen_height >> 8) & 0xFF, screen_height & 0xFF};
        lcd_data(data, 4);
    }
    
    lcd_cmd(0x2C);    // Memory write
    for (int y = 0; y < screen_height; y++) {
        for (int x = 0; x < screen_width; x += 32) {
            lcd_data((uint8_t*)buffer, MIN(32, screen_width - x) * 2);
        }
    }
}

void display_draw_text(int16_t x, int16_t y, const char *text, uint16_t color, uint16_t bg, font_size_t size) {
    while (*text) {
        display_draw_char(x, y, *text, color, bg, size);
        x += 8 * size;  // Advance cursor
        text++;
    }
}

void display_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, font_size_t size) {
    for (int8_t i = 0; i < 8; i++) {
        uint8_t line = font8x8_basic[(unsigned char)c][i];
        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                display_fill_rect(x + j * size, y + i * size, size, size, color);
            } else {
                display_fill_rect(x + j * size, y + i * size, size, size, bg);
            }
            line >>= 1;
        }
    }
}

void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w < 1 || h < 1) return;
    
    lcd_cmd(0x2A);    // Column address set
    {
        uint8_t data[] = {
            (x >> 8) & 0xFF, x & 0xFF,
            ((x + w - 1) >> 8) & 0xFF, (x + w - 1) & 0xFF
        };
        lcd_data(data, 4);
    }
    
    lcd_cmd(0x2B);    // Row address set
    {
        uint8_t data[] = {
            (y >> 8) & 0xFF, y & 0xFF,
            ((y + h - 1) >> 8) & 0xFF, (y + h - 1) & 0xFF
        };
        lcd_data(data, 4);
    }
    
    lcd_cmd(0x2C);    // Memory write
    uint16_t buffer[32];
    for (int i = 0; i < 32; i++) {
        buffer[i] = color;
    }
    
    for (int16_t i = 0; i < h; i++) {
        lcd_data((uint8_t*)buffer, w * 2);
    }
}

void display_show_alert(const char *message) {
    display_fill_screen(COLOR_BLACK);
    display_draw_text(10, 10, "ALERT:", COLOR_RED, COLOR_BLACK, FONT_LARGE);
    display_draw_text(10, 40, message, COLOR_WHITE, COLOR_BLACK, FONT_MEDIUM);
}

void display_clear(void) {
    display_fill_screen(COLOR_BLACK);
}