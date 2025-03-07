// components/network_module/network_challenges.c
#include "network_challenges.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "network_challenges";

// Current active challenge
static network_challenge_type_t active_challenge = -1;
static TaskHandle_t challenge_task_handle = NULL;

// Queue for packet analysis
static QueueHandle_t packet_queue = NULL;

// Callback function for WiFi promiscuous mode
static void wifi_promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
    wifi_packet_t *pkt = (wifi_packet_t *)ppkt->payload;
    
    // Send packet to queue for analysis
    if (xQueueSend(packet_queue, pkt, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Packet queue full!");
    }
}

// Task to handle beacon frame analysis
static void beacon_analysis_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting Beacon Analysis Challenge");
    
    // Configure promiscuous mode
    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
    };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    wifi_packet_t pkt;
    while (active_challenge == NET_CHALLENGE_BEACON_ANALYSIS) {
        if (xQueueReceive(packet_queue, &pkt, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Analyze only beacon frames
            if (pkt.hdr.frame_ctrl.type == WIFI_FRAME_TYPE_MGMT && 
                pkt.hdr.frame_ctrl.subtype == WIFI_MGMT_SUBTYPE_BEACON) {
                
                ESP_LOGI(TAG, "Beacon Frame Detected:");
                ESP_LOGI(TAG, "BSSID: " MACSTR, MAC2STR(pkt.hdr.addr3));
                ESP_LOGI(TAG, "SSID: %.*s", pkt.beacon.ssid_length, pkt.beacon.ssid);
                ESP_LOGI(TAG, "Channel: %d", ppkt->rx_ctrl.channel);
                ESP_LOGI(TAG, "RSSI: %d", ppkt->rx_ctrl.rssi);
            }
        }
    }

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    vTaskDelete(NULL);
}

// Task to handle protocol security challenge
static void protocol_security_task(void *pvParameters) {
    // Simulate different security protocols
    const char *security_types[] = {
        "Open (No Security)",
        "WEP",
        "WPA-PSK",
        "WPA2-PSK",
        "WPA3"
    };

    while (active_challenge == NET_CHALLENGE_PROTOCOL_SECURITY) {
        for (int i = 0; i < 5; i++) {
            ESP_LOGI(TAG, "Demonstrating %s:", security_types[i]);
            // Show security features and potential vulnerabilities
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    vTaskDelete(NULL);
}

// Task to handle evil twin detection challenge
static void evil_twin_task(void *pvParameters) {
    // Create a list of legitimate networks
    network_info_t legitimate_networks[] = {
        {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}, "CorporateWiFi", 1, WIFI_AUTH_WPA2_PSK, -55},
        {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}, "GuestNetwork", 6, WIFI_AUTH_WPA2_PSK, -60}
    };

    // Simulate detection of suspicious networks
    while (active_challenge == NET_CHALLENGE_EVIL_TWIN) {
        for (int i = 0; i < 2; i++) {
            // Simulate finding a potential evil twin
            network_info_t suspicious = legitimate_networks[i];
            suspicious.bssid[5] ^= 0x01;  // Slightly modified BSSID
            suspicious.rssi = -45;        // Stronger signal

            ESP_LOGI(TAG, "Suspicious network detected:");
            ESP_LOGI(TAG, "SSID: %s", suspicious.ssid);
            ESP_LOGI(TAG, "Original BSSID: " MACSTR, MAC2STR(legitimate_networks[i].bssid));
            ESP_LOGI(TAG, "Suspicious BSSID: " MACSTR, MAC2STR(suspicious.bssid));
            
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }

    vTaskDelete(NULL);
}

esp_err_t network_challenges_init(void) {
    packet_queue = xQueueCreate(32, sizeof(wifi_promiscuous_pkt_t));
    if (packet_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create packet queue");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Network challenges module initialized");
    return ESP_OK;
}

esp_err_t start_network_challenge(network_challenge_type_t type) {
    if (active_challenge != -1) {
        ESP_LOGE(TAG, "Challenge already running");
        return ESP_ERR_INVALID_STATE;
    }

    active_challenge = type;
    
    switch (type) {
        case NET_CHALLENGE_BEACON_ANALYSIS:
            xTaskCreate(beacon_analysis_task, "beacon_analysis", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case NET_CHALLENGE_PROTOCOL_SECURITY:
            xTaskCreate(protocol_security_task, "protocol_security", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case NET_CHALLENGE_EVIL_TWIN:
            xTaskCreate(evil_twin_task, "evil_twin", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        default:
            ESP_LOGE(TAG, "Unknown challenge type");
            return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Started network challenge type %d", type);
    return ESP_OK;
}

esp_err_t stop_network_challenge(void) {
    if (active_challenge == -1) {
        return ESP_OK;
    }

    active_challenge = -1;
    vTaskDelay(pdMS_TO_TICKS(100));  // Give time for task to clean up

    if (challenge_task_handle != NULL) {
        vTaskDelete(challenge_task_handle);
        challenge_task_handle = NULL;
    }

    ESP_LOGI(TAG, "Stopped network challenge");
    return ESP_OK;
}