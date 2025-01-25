// components/display/ssd1306.c
#include "ssd1306.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SSD1306";

// SSD1306 commands
#define SSD1306_CMD_SET_CONTRAST            0x81
#define SSD1306_CMD_DISPLAY_RAM             0xA4
#define SSD1306_CMD_DISPLAY_NORMAL          0xA6
#define SSD1306_CMD_DISPLAY_OFF             0xAE
#define SSD1306_CMD_DISPLAY_ON              0xAF
#define SSD1306_CMD_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_CMD_SET_COM_PINS            0xDA
#define SSD1306_CMD_SET_VCOM_DETECT         0xDB
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIV   0xD5
#define SSD1306_CMD_SET_PRECHARGE           0xD9
#define SSD1306_CMD_SET_MULTIPLEX           0xA8
#define SSD1306_CMD_SET_LOW_COLUMN          0x00
#define SSD1306_CMD_SET_HIGH_COLUMN         0x10
#define SSD1306_CMD_SET_START_LINE          0x40
#define SSD1306_CMD_SET_MEMORY_MODE         0x20
#define SSD1306_CMD_SET_PAGE_ADDRESS        0xB0
#define SSD1306_CMD_SET_COM_SCAN_INC        0xC0
#define SSD1306_CMD_SET_COM_SCAN_DEC        0xC8
#define SSD1306_CMD_SET_SEGMENT_REMAP       0xA0
#define SSD1306_CMD_SET_CHARGE_PUMP         0x8D

// Display dimensions
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_PAGES       (SSD1306_HEIGHT / 8)

// Structure to hold device information
typedef struct ssd1306_dev_t {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_PAGES];
} ssd1306_dev_t;

// Font data (basic 8x8 font)
static const uint8_t font8x8_basic[96][8] = {
    // First 32 characters (0x20-0x3F) - space, symbols, numbers
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x00, 0x4F, 0x4F, 0x00, 0x00, 0x00}, // !
    // ... Add more font data here
};

// Helper function to write command to display
static esp_err_t ssd1306_write_cmd(ssd1306_handle_t dev, uint8_t cmd) {
    uint8_t write_buf[2] = {0x00, cmd}; // First byte 0x00 indicates command
    return i2c_master_write_to_device(dev->i2c_port, dev->i2c_addr, 
                                    write_buf, sizeof(write_buf), 
                                    pdMS_TO_TICKS(10));
}

// Helper function to write data to display
static esp_err_t ssd1306_write_data(ssd1306_handle_t dev, uint8_t* data, size_t size) {
    // Allocate buffer for command byte (0x40) + data
    uint8_t *write_buf = malloc(size + 1);
    if (write_buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    write_buf[0] = 0x40; // Data mode
    memcpy(write_buf + 1, data, size);
    
    esp_err_t ret = i2c_master_write_to_device(dev->i2c_port, dev->i2c_addr,
                                             write_buf, size + 1,
                                             pdMS_TO_TICKS(10));
    free(write_buf);
    return ret;
}

// Initialize display with default settings
static esp_err_t ssd1306_init(ssd1306_handle_t dev) {
    esp_err_t ret;
    
    // Initialization sequence
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_OFF);
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_DISPLAY_CLOCK_DIV);
    ret |= ssd1306_write_cmd(dev, 0x80); // Suggested ratio
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_MULTIPLEX);
    ret |= ssd1306_write_cmd(dev, SSD1306_HEIGHT - 1);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_DISPLAY_OFFSET);
    ret |= ssd1306_write_cmd(dev, 0x00);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_START_LINE | 0x00);
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_CHARGE_PUMP);
    ret |= ssd1306_write_cmd(dev, 0x14); // Enable charge pump
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_MEMORY_MODE);
    ret |= ssd1306_write_cmd(dev, 0x00); // Horizontal addressing mode
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_SEGMENT_REMAP | 0x01);
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_SCAN_DEC);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_PINS);
    ret |= ssd1306_write_cmd(dev, 0x12);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_CONTRAST);
    ret |= ssd1306_write_cmd(dev, 0xCF);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_PRECHARGE);
    ret |= ssd1306_write_cmd(dev, 0xF1);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_VCOM_DETECT);
    ret |= ssd1306_write_cmd(dev, 0x40);
    
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_RAM);
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_NORMAL);
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_ON);
    
    return ret;
}

ssd1306_handle_t ssd1306_create(i2c_port_t i2c_port, uint8_t i2c_addr) {
    ssd1306_dev_t *dev = calloc(1, sizeof(ssd1306_dev_t));
    if (dev == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for device");
        return NULL;
    }
    
    dev->i2c_port = i2c_port;
    dev->i2c_addr = i2c_addr;
    
    if (ssd1306_init(dev) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display");
        free(dev);
        return NULL;
    }
    
    return (ssd1306_handle_t)dev;
}

esp_err_t ssd1306_clear_screen(ssd1306_handle_t dev, uint8_t fill_data) {
    memset(dev->buffer, fill_data, sizeof(dev->buffer));
    return ESP_OK;
}

esp_err_t ssd1306_refresh_gram(ssd1306_handle_t dev) {
    esp_err_t ret;
    
    // Set column address
    ret = ssd1306_write_cmd(dev, SSD1306_CMD_SET_LOW_COLUMN | 0x0);
    ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_HIGH_COLUMN | 0x0);
    
    // Set page address
    for (int i = 0; i < SSD1306_PAGES; i++) {
        ret |= ssd1306_write_cmd(dev, SSD1306_CMD_SET_PAGE_ADDRESS | i);
        ret |= ssd1306_write_data(dev, &dev->buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
    
    return ret;
}

esp_err_t ssd1306_draw_pixel(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint16_t byte_idx = x + (y / 8) * SSD1306_WIDTH;
    uint8_t bit_idx = y % 8;
    
    if (color) {
        dev->buffer[byte_idx] |= (1 << bit_idx);
    } else {
        dev->buffer[byte_idx] &= ~(1 << bit_idx);
    }
    
    return ESP_OK;
}

esp_err_t ssd1306_draw_string(ssd1306_handle_t dev, uint8_t x, uint8_t y,
                             const char* text, uint8_t font_size, uint8_t color) {
    if (text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t cursor_x = x;
    uint8_t cursor_y = y;
    
    while (*text) {
        // Check if character is printable
        if (*text >= ' ' && *text <= '~') {
            const uint8_t *char_data = font8x8_basic[*text - ' '];
            
            // Draw character
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    if (char_data[row] & (1 << col)) {
                        for (int size_y = 0; size_y < font_size; size_y++) {
                            for (int size_x = 0; size_x < font_size; size_x++) {
                                ssd1306_draw_pixel(dev,
                                                 cursor_x + col * font_size + size_x,
                                                 cursor_y + row * font_size + size_y,
                                                 color);
                            }
                        }
                    }
                }
            }
            
            cursor_x += 8 * font_size;
        }
        text++;
    }
    
    return ESP_OK;
}

esp_err_t ssd1306_display_on(ssd1306_handle_t dev, bool on) {
    return ssd1306_write_cmd(dev, on ? SSD1306_CMD_DISPLAY_ON : SSD1306_CMD_DISPLAY_OFF);
}

esp_err_t ssd1306_fill_rectangle(ssd1306_handle_t dev, uint8_t x, uint8_t y,
                                uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t i = 0; i < w; i++) {
        for (uint8_t j = 0; j < h; j++) {
            ssd1306_draw_pixel(dev, x + i, y + j, color);
        }
    }
    return ESP_OK;
}

esp_err_t ssd1306_draw_rectangle(ssd1306_handle_t dev, uint8_t x, uint8_t y,
                                uint8_t w, uint8_t h, uint8_t color) {
    // Draw horizontal lines
    for (uint8_t i = 0; i < w; i++) {
        ssd1306_draw_pixel(dev, x + i, y, color);
        ssd1306_draw_pixel(dev, x + i, y + h - 1, color);
    }
    
    // Draw vertical lines
    for (uint8_t i = 0; i < h; i++) {
        ssd1306_draw_pixel(dev, x, y + i, color);
        ssd1306_draw_pixel(dev, x + w - 1, y + i, color);
    }
    return ESP_OK;
}