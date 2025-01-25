# Main CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Include the components directory for ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set your project name
project(esp32_security_trainer)

# Component CMakeLists.txt (in components/display)
idf_component_register(
    SRCS 
        "display.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        "driver"
        "esp_lcd"
)

# Component CMakeLists.txt (in components/network_module)
idf_component_register(
    SRCS 
        "network_module.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        "esp_wifi"
        "esp_netif"
        "nvs_flash"
)

# Component CMakeLists.txt (in components/web_module)
idf_component_register(
    SRCS 
        "web_module.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        "esp_http_server"
        "esp_https_server"
)

# Component CMakeLists.txt (in components/hardware_module)
idf_component_register(
    SRCS 
        "hardware_module.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        "driver"
        "esp_hw_support"
)