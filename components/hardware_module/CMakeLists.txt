# components/hardware_module/CMakeLists.txt
idf_component_register(
    SRCS "hardware_challenges.c"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "esp_adc" "esp_timer" "nvs_flash"
)