# components/bluetooth_module/CMakeLists.txt
idf_component_register(
    SRCS "bluetooth_challenges.c"
    INCLUDE_DIRS "include"
    REQUIRES "bt" "nvs_flash" "esp_timer" "esp_hw_support" 
)

# Add chip-specific include paths
target_include_directories(${COMPONENT_LIB} PUBLIC
    "${IDF_PATH}/components/bt/include/${IDF_TARGET}/include"
    "${IDF_PATH}/components/bt/host/bluedroid/api/include/api"
)
