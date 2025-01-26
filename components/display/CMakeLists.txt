# components/display/CMakeLists.txt
idf_component_register(
    SRCS "display.c" 
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_lcd_ili9341 hal esp_rom esp_common lvgl
)

set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/components
)

set(DEPENDENCIES
    lvgl
)