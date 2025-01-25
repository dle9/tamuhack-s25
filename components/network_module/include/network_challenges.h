// components/network_module/include/network_challenges.h
#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

// WiFi frame type and subtype definitions
#define WIFI_MGMT_SUBTYPE_BEACON       0x08
#define WIFI_MGMT_SUBTYPE_PROBE_REQ    0x04
#define WIFI_MGMT_SUBTYPE_PROBE_RES    0x05
#define WIFI_MGMT_SUBTYPE_DEAUTH       0x0C

// Frame type definitions
typedef enum {
    WIFI_FRAME_TYPE_MGMT = 0,
    WIFI_FRAME_TYPE_CTRL = 1,
    WIFI_FRAME_TYPE_DATA = 2
} wifi_frame_type_t;

// Frame control field structure
typedef struct {
    uint16_t protocol: 2;
    uint16_t type: 2;
    uint16_t subtype: 4;
    uint16_t to_ds: 1;
    uint16_t from_ds: 1;
    uint16_t more_frag: 1;
    uint16_t retry: 1;
    uint16_t pwr_mgmt: 1;
    uint16_t more_data: 1;
    uint16_t protected_frame: 1;
    uint16_t order: 1;
} __attribute__((packed)) wifi_frame_ctrl_t;

// Network info structure
typedef struct {
    uint8_t bssid[6];     // MAC address
    char ssid[33];        // 32 chars + null terminator
    uint8_t channel;      // WiFi channel
    wifi_auth_mode_t auth_mode; // Authentication mode
    int8_t rssi;         // Signal strength
} network_info_t;

// MAC header structure
typedef struct {
    wifi_frame_ctrl_t frame_ctrl;
    uint16_t duration;
    uint8_t addr1[6]; // Destination address
    uint8_t addr2[6]; // Source address
    uint8_t addr3[6]; // BSSID for most frames
    uint16_t sequence_ctrl;
} __attribute__((packed)) wifi_mac_hdr_t;

// Beacon packet structure
typedef struct {
    wifi_mac_hdr_t hdr;
    union {
        struct {
            uint64_t timestamp;
            uint16_t beacon_interval;
            uint16_t capability;
            uint8_t ssid_length;
            uint8_t ssid[32];
        } __attribute__((packed)) beacon;
        uint8_t payload[0];
    };
} __attribute__((packed)) wifi_beacon_packet_t;

// Challenge types
typedef enum {
    NET_CHALLENGE_BEACON_ANALYSIS,
    NET_CHALLENGE_PACKET_ANALYSIS,
    NET_CHALLENGE_PROTOCOL_SECURITY,
    NET_CHALLENGE_DEAUTH_DETECTION,
    NET_CHALLENGE_EVIL_TWIN
} network_challenge_type_t;

// Function declarations
esp_err_t network_challenges_init(void);
esp_err_t start_network_challenge(network_challenge_type_t type);
esp_err_t stop_network_challenge(void);
esp_err_t get_challenge_status(void* status_buffer, size_t buffer_size);