// components/hardware_module/include/hardware_challenges.h
#pragma once

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"
#include "esp_efuse.h"
#include "spi_flash_mmap.h"
#include "hal/temp_sensor_hal.h"

// Challenge types for different hardware security scenarios
typedef enum {
    HW_CHALLENGE_TIMING_ATTACK,      // Demonstrate timing-based vulnerabilities
    HW_CHALLENGE_VOLTAGE_GLITCH,     // Show voltage glitching detection
    HW_CHALLENGE_SECURE_BOOT,        // Demonstrate secure boot concepts
    HW_CHALLENGE_SIDE_CHANNEL,       // Power analysis and side-channel attacks
    HW_CHALLENGE_SECURE_STORAGE      // Secure storage implementation
} hardware_challenge_type_t;

// Function declarations
esp_err_t hardware_challenges_init(void);
esp_err_t start_hardware_challenge(hardware_challenge_type_t type);
esp_err_t stop_hardware_challenge(void);
esp_err_t get_hardware_challenge_status(void* status_buffer, size_t buffer_size);