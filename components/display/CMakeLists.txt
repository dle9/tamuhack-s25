# components/display/CMakeLists.txt
idf_component_register(
    SRCS 
        "display.c"
        "ssd1306.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        "driver"
        "esp_lcd"
        "esp_driver_i2c"
)