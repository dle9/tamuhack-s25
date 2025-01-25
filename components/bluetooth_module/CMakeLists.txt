# components/bluetooth_module/CMakeLists.txt
idf_component_register(
    SRCS "bluetooth_challenges.c"
    INCLUDE_DIRS "include"
    REQUIRES "bt" "nvs_flash" "esp_timer"
)