// In display.c
#include "display.h"  // Include this header to define display_config_t
#include <string.h>
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"  // Add this header for ESP_LOGI macro
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "lvgl.h"
#include "esp_timer.h"

// Global variables to track display state
static lv_display_t *g_disp = NULL;
static esp_lcd_panel_handle_t g_panel_handle = NULL;
static const char *TAG = "display";

// LVGL Tick Timer Callback
static void lvgl_tick_callback(void *arg) {
    lv_tick_inc(1);
}

// Flush Callback for LVGL
static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    
    // Swap RGB bytes for SPI LCD
    lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
    
    // Draw bitmap
    esp_lcd_panel_draw_bitmap(
        panel_handle, 
        area->x1, area->y1, 
        area->x2 + 1, area->y2 + 1, 
        px_map
    );
    
    // Mark flush complete
    lv_display_flush_ready(disp);
}

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
    ESP_ERROR_CHECK(spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO));

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
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->spi_host, &io_config, &io_handle));

    // Panel Configuration
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->rst_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &g_panel_handle));

    // Panel Initialization
    ESP_ERROR_CHECK(esp_lcd_panel_reset(g_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(g_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(g_panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(g_panel_handle, true));

    // Backlight Configuration
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << config->backlight_pin,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(config->backlight_pin, 1);

    // Initialize LVGL
    lv_init();

    // Create LVGL display
    g_disp = lv_display_create(config->width, config->height);
    if (!g_disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_FAIL;
    }

    // Allocate draw buffers
    size_t draw_buf_lines = 20;
    size_t draw_buf_size = config->width * draw_buf_lines * sizeof(lv_color_t);
    
    lv_color_t *draw_buf1 = heap_caps_malloc(draw_buf_size, MALLOC_CAP_DMA);
    lv_color_t *draw_buf2 = heap_caps_malloc(draw_buf_size, MALLOC_CAP_DMA);

    if (!draw_buf1 || !draw_buf2) {
        ESP_LOGE(TAG, "Failed to allocate draw buffers");
        if (draw_buf1) free(draw_buf1);
        if (draw_buf2) free(draw_buf2);
        return ESP_ERR_NO_MEM;
    }

    memset(draw_buf1, 0, draw_buf_size);
    memset(draw_buf2, 0, draw_buf_size);

    // Set display buffers
    lv_display_set_buffers(
        g_disp, 
        draw_buf1, 
        draw_buf2, 
        draw_buf_size, 
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    // Set display properties
    lv_display_set_flush_cb(g_disp, example_lvgl_flush_cb);
    lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(g_disp, g_panel_handle);

    // Create timer for LVGL tick
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = lvgl_tick_callback,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1000)); // 1ms

    // Create a default screen with black background
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    return ESP_OK;
}

// Modify display_show_menu
void display_show_menu(const menu_item_t *items, size_t num_items, size_t selected) 
{
    // Ensure we have a valid display
    if (!g_disp) return;

    // Clear existing screen
    lv_obj_clean(lv_screen_active());

    // Create a new screen
    lv_obj_t *scr = lv_screen_active();
    
    // Ensure black background
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Create title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32 Security Trainer");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, lv_pct(100));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Create menu items
    for (size_t i = 0; i < num_items; i++) {
        lv_obj_t *btn = lv_button_create(scr);
        lv_obj_set_width(btn, lv_pct(80));
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 50 + (i * 50));

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i].name);
        lv_obj_center(label);

        // Highlight selected item
        if (i == selected) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x00FF00), 0);  // Green
            lv_obj_set_style_text_color(label, lv_color_black(), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x808080), 0);  // Gray
            lv_obj_set_style_text_color(label, lv_color_white(), 0);
        }
    }

    // Force screen update
    lv_refr_now(NULL);
}