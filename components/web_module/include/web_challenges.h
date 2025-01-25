// components/web_module/include/web_challenges.h
#pragma once

#include "esp_http_server.h"
#include "esp_err.h"

// Challenge status structure
typedef struct {
    bool completed;
    uint32_t attempts;
    uint32_t start_time;
} challenge_status_t;

// Challenge difficulty levels
typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
} challenge_difficulty_t;

// Initialize the web challenges module
esp_err_t web_challenges_init(void);

// Start a specific challenge
esp_err_t start_challenge(uint8_t challenge_id);

// Get challenge status
challenge_status_t get_challenge_status(uint8_t challenge_id);
