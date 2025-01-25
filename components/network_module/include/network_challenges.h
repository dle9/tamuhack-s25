// components/network_module/include/network_challenges.h
#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

// Challenge types for different network security scenarios
typedef enum {
    NET_CHALLENGE_BEACON_ANALYSIS,    // Analyze beacon frames and network types
    NET_CHALLENGE_PACKET_ANALYSIS,    // Analyze network traffic patterns
    NET_CHALLENGE_PROTOCOL_SECURITY,  // Study different protocol securities
    NET_CHALLENGE_DEAUTH_DETECTION,   // Detect deauthentication attacks
    NET_CHALLENGE_EVIL_TWIN          // Identify rogue access points
} network_challenge_type_t;

// Network challenge configuration structure
typedef struct {
    network_challenge_type_t type;
    uint8_t difficulty;
    bool logging_enabled;
    void (*callback)(void* arg);
} network_challenge_config_t;

// Initialize the network challenges module
esp_err_t network_challenges_init(void);

// Start a specific network challenge
esp_err_t start_network_challenge(network_challenge_type_t type);

// Stop the current challenge
esp_err_t stop_network_challenge(void);

// Get the current challenge status
esp_err_t get_challenge_status(void* status_buffer, size_t buffer_size);
