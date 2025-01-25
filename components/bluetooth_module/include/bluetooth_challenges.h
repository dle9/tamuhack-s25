// components/bluetooth_module/include/bluetooth_challenges.h
#pragma once

#include "esp_err.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

// Types of Bluetooth security challenges
typedef enum {
    BT_CHALLENGE_SCANNING,           // Demonstrate BLE scanning and device discovery
    BT_CHALLENGE_PAIRING,           // Analyze different pairing mechanisms
    BT_CHALLENGE_MAN_IN_MIDDLE,     // Show MITM attack scenarios
    BT_CHALLENGE_SNIFFING,          // Practice packet sniffing and analysis
    BT_CHALLENGE_SPOOFING           // Demonstrate device spoofing detection
} bluetooth_challenge_type_t;

// Challenge configuration structure
typedef struct {
    bluetooth_challenge_type_t type;
    uint8_t difficulty;
    bool logging_enabled;
    void (*callback)(void* arg);
} bluetooth_challenge_config_t;

// Initialize Bluetooth security module
esp_err_t bluetooth_challenges_init(void);

// Start a specific Bluetooth challenge
esp_err_t start_bluetooth_challenge(bluetooth_challenge_type_t type);

// Stop the current challenge
esp_err_t stop_bluetooth_challenge(void);

// Get current challenge status
esp_err_t get_bluetooth_challenge_status(void* status_buffer, size_t buffer_size);
