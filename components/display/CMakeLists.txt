# components/display/CMakeLists.txt
idf_component_register(
    SRCS "display.c" "font8x8_basic.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_lcd_ili9341 hal esp_rom esp_common
)