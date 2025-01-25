// components/web_module/include/web_challenges.h
#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

// Challenge difficulty levels
typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
} challenge_difficulty_t;

// Challenge status tracking
typedef struct {
    uint32_t start_time;
    uint16_t attempts;
    bool completed;
} web_challenge_status_t;  

// Web challenge types
typedef enum {
    WEB_CHALLENGE_AUTH = 0,
    WEB_CHALLENGE_SQLI,
    WEB_CHALLENGE_XSS
} web_challenge_type_t;

// Function declarations with renamed functions to avoid conflicts
esp_err_t web_challenges_init(void);
esp_err_t start_web_challenge(web_challenge_type_t challenge_type);
web_challenge_status_t web_get_challenge_status(web_challenge_type_t challenge_type);