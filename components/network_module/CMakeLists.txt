# components/network_module/CMakeLists.txt
idf_component_register(
    SRCS "network_challenges.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_wifi" "esp_event" "nvs_flash" "esp_netif" "esp_hw_support"
)