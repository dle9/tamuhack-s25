// components/display/display.c
#include "display.h"
#include "driver/gpio.h"  // Add this header for gpio_config_t
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_heap_caps.h"  // For heap_caps_malloc


static const char *TAG = "display";

// Global variables for display
static lv_display_t *disp = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

// LVGL draw buffers
#define LVGL_DRAW_BUFFER_LINES 20
static lv_color_t *draw_buf1 = NULL;
static lv_color_t *draw_buf2 = NULL;

        // LVGL flush callback
static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    
    // Because SPI LCD is big-endian, we need to swap the RGB bytes order
    lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
    
    // Copy buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, 
        area->x1, area->y1, 
        area->x2 + 1, area->y2 + 1, 
        px_map);
    
    // Mark flush as complete
    lv_display_flush_is_last(disp);
}

// Display initialization
esp_err_t display_init(display_config_t *config) 
{
    ESP_LOGI(TAG, "Initializing Display");

    // SPI Bus Configuration
    spi_bus_config_t buscfg = {
        .sclk_io_num = config->sclk_pin,
        .mosi_io_num = config->mosi_pin,
        .miso_io_num = config->miso_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = config->width * 80 * sizeof(uint16_t),
    };
    esp_err_t ret = spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Panel IO Configuration
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = config->dc_pin,
        .cs_gpio_num = config->cs_pin,
        .pclk_hz = config->clock_speed,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->spi_host, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Panel Configuration
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->rst_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ret = esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Panel Initialization
    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_mirror(panel_handle, true, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mirror panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn on display: %s", esp_err_to_name(ret));
        return ret;
    }

    // Backlight Configuration
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << config->backlight_pin,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret = gpio_config(&bk_gpio_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure backlight GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    gpio_set_level(config->backlight_pin, 1);

    // Initialize LVGL
    lv_init();

    // Create LVGL Display
    disp = lv_display_create(config->width, config->height);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_FAIL;
    }

    // Allocate Draw Buffers
    size_t draw_buffer_sz = config->width * LVGL_DRAW_BUFFER_LINES * sizeof(lv_color_t);
    
    // Use heap allocation for draw buffers with DMA capability
    draw_buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);
    draw_buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);

    if (!draw_buf1 || !draw_buf2) {
        ESP_LOGE(TAG, "Failed to allocate draw buffers");
        return ESP_ERR_NO_MEM;
    }

    // Set LVGL Display Buffers
    lv_display_set_buffers(disp, draw_buf1, draw_buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(disp, panel_handle);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, example_lvgl_flush_cb);

    // Ensure initial screen is cleared
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    return ESP_OK;
}


// Menu display using LVGL
void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected) 
{
    // Clear existing screen
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Set background color to ensure visibility
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Create a title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32 Security Trainer");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Create a list for menu items
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, lv_pct(90), lv_pct(80));
    lv_obj_center(list);

    // Add menu items to the list
    for (size_t i = 0; i < num_items; i++) {
        lv_obj_t *btn = lv_list_add_btn(list, NULL, items[i].name);
        
        // Set text color
        lv_obj_set_style_text_color(btn, 
            i == selected ? lv_color_hex(0x00FF00) : lv_color_white(), 
            0
        );
        
        // Highlight the currently selected item
        if (i == selected) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        }
    }

    // Force screen update
    lv_refr_now(disp);
}

// Optional: Add a show alert function using LVGL
void display_show_alert(const char *message) 
{
    // Clear existing screen
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Create an alert label
    lv_obj_t *alert_label = lv_label_create(scr);
    lv_label_set_text(alert_label, message);
    lv_obj_set_style_text_color(alert_label, lv_color_hex(0xFF0000), 0);  // Red color
    lv_obj_align(alert_label, LV_ALIGN_CENTER, 0, 0);
}

// Clear screen function
void display_clear(void) 
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
}