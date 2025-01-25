# components/web_module/CMakeLists.txt
idf_component_register(
    SRCS "web_challenges.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_http_server" "esp_wifi" "nvs_flash" "esp_netif"
    PRIV_REQUIRES "json"    # Added json as a private requirement
)