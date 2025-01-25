// components/network_module/include/network_challenges.h
#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

// Define IEEE 802.11 frame control field structure
typedef struct {
    unsigned protocol: 2;
    unsigned type: 2;
    unsigned subtype: 4;
    unsigned to_ds: 1;
    unsigned from_ds: 1;
    unsigned more_frag: 1;
    unsigned retry: 1;
    unsigned pwr_mgmt: 1;
    unsigned more_data: 1;
    unsigned wep: 1;
    unsigned order: 1;
} wifi_ieee80211_frame_ctrl_t;

// Define IEEE 802.11 MAC header structure
typedef struct {
    wifi_ieee80211_frame_ctrl_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6]; // Destination address
    uint8_t addr2[6]; // Source address
    uint8_t addr3[6]; // BSSID
    uint16_t sequence_ctrl;
} wifi_ieee80211_mac_hdr_t;

// Define complete IEEE 802.11 packet structure
typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; // Variable length payload
} wifi_ieee80211_packet_t;

// Challenge types
typedef enum {
    NET_CHALLENGE_BEACON_ANALYSIS,
    NET_CHALLENGE_PACKET_ANALYSIS,
    NET_CHALLENGE_PROTOCOL_SECURITY,
    NET_CHALLENGE_DEAUTH_DETECTION,
    NET_CHALLENGE_EVIL_TWIN
} network_challenge_type_t;

// Function declarations remain the same...
esp_err_t network_challenges_init(void);
esp_err_t start_network_challenge(network_challenge_type_t type);
esp_err_t stop_network_challenge(void);
esp_err_t get_challenge_status(void* status_buffer, size_t buffer_size);