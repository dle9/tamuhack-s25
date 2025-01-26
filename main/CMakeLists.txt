idf_component_register(SRCS "softap_sta.c"
                    PRIV_REQUIRES esp_wifi nvs_flash protocol_examples_common esp_eth esp-tls nvs_flash esp_netif esp_http_server

                    INCLUDE_DIRS ".")
