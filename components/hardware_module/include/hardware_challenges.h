// components/hardware_module/include/hardware_challenges.h
#pragma once

#include "esp_err.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"

// Types of hardware security challenges
typedef enum {
    HW_CHALLENGE_TIMING_ATTACK,      // Demonstrate timing-based vulnerabilities
    HW_CHALLENGE_VOLTAGE_GLITCH,     // Show voltage glitching detection
    HW_CHALLENGE_SECURE_BOOT,        // Demonstrate secure boot concepts
    HW_CHALLENGE_SIDE_CHANNEL,       // Power analysis and side-channel attacks
    HW_CHALLENGE_SECURE_STORAGE      // Secure storage implementation
} hardware_challenge_type_t;

// Configuration for hardware challenges
typedef struct {
    hardware_challenge_type_t type;
    uint8_t difficulty;              // 1-5, where 5 is most difficult
    bool logging_enabled;
    void (*callback)(void* arg);     // Callback for challenge events
} hardware_challenge_config_t;

// Initialize hardware security module
esp_err_t hardware_challenges_init(void);

// Start a specific hardware challenge
esp_err_t start_hardware_challenge(hardware_challenge_type_t type);

// Stop the current challenge
esp_err_t stop_hardware_challenge(void);

// Get current challenge status
esp_err_t get_hardware_challenge_status(void* status_buffer, size_t buffer_size);

