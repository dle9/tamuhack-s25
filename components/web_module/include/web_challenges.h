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

// components/web_module/web_challenges.c
#include "web_challenges.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "web_challenges";

// Web server handle
static httpd_handle_t server = NULL;

// Challenge definitions
typedef struct {
    const char *name;
    const char *description;
    challenge_difficulty_t difficulty;
    httpd_uri_t endpoint;
    challenge_status_t status;
} challenge_def_t;

// Simulated user database for authentication challenges
typedef struct {
    const char *username;
    const char *password_hash;
    const char *role;
} user_entry_t;

static const user_entry_t DEMO_USERS[] = {
    {"admin", "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8", "admin"},
    {"user", "e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446", "user"}
};

// Authentication challenge handler
static esp_err_t auth_challenge_handler(httpd_req_t *req) {
    char *buf = malloc(req->content_len + 1);
    size_t received = httpd_req_recv(req, buf, req->content_len);
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    const char *username = cJSON_GetObjectItem(root, "username")->valuestring;
    const char *password = cJSON_GetObjectItem(root, "password")->valuestring;

    // For training purposes, we're using basic authentication
    // In real applications, you'd use proper security measures
    bool auth_success = false;
    for (size_t i = 0; i < sizeof(DEMO_USERS)/sizeof(DEMO_USERS[0]); i++) {
        if (strcmp(username, DEMO_USERS[i].username) == 0) {
            // In real applications, never store or compare plain passwords
            if (strcmp(password, "password123") == 0) {
                auth_success = true;
                break;
            }
        }
    }

    const char *response;
    if (auth_success) {
        response = "{\"status\":\"success\",\"message\":\"Authentication successful\"}";
    } else {
        response = "{\"status\":\"error\",\"message\":\"Invalid credentials\"}";
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));

    free(buf);
    cJSON_Delete(root);
    return ESP_OK;
}

// SQL Injection challenge handler
static esp_err_t sqli_challenge_handler(httpd_req_t *req) {
    char *buf = malloc(req->content_len + 1);
    size_t received = httpd_req_recv(req, buf, req->content_len);
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    const char *user_input = cJSON_GetObjectItem(root, "query")->valuestring;

    // Simulate vulnerable SQL query
    // WARNING: This is intentionally vulnerable for training purposes
    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM users WHERE id = %s", user_input);

    // Demonstrate SQL injection vulnerability
    bool injection_detected = strstr(user_input, "'") != NULL || 
                            strstr(user_input, "\"") != NULL || 
                            strstr(user_input, ";") != NULL;

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "query", query);
    if (injection_detected) {
        cJSON_AddStringToObject(response, "hint", "SQL injection detected! Can you bypass the authentication?");
    }

    char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));

    free(buf);
    free(resp_str);
    cJSON_Delete(root);
    cJSON_Delete(response);
    return ESP_OK;
}

// XSS challenge handler
static esp_err_t xss_challenge_handler(httpd_req_t *req) {
    char *buf = malloc(req->content_len + 1);
    size_t received = httpd_req_recv(req, buf, req->content_len);
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    const char *user_input = cJSON_GetObjectItem(root, "message")->valuestring;

    // Intentionally vulnerable HTML response
    // WARNING: This is for training purposes only
    char response[512];
    snprintf(response, sizeof(response),
             "<html><body><h1>Guest Book</h1><p>Latest message: %s</p></body></html>",
             user_input);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response, strlen(response));

    free(buf);
    cJSON_Delete(root);
    return ESP_OK;
}

// Challenge definitions array
static challenge_def_t challenges[] = {
    {
        .name = "Basic Authentication",
        .description = "Learn about authentication vulnerabilities",
        .difficulty = DIFFICULTY_EASY,
        .endpoint = {
            .uri = "/auth",
            .method = HTTP_POST,
            .handler = auth_challenge_handler
        }
    },
    {
        .name = "SQL Injection",
        .description = "Practice SQL injection detection and prevention",
        .difficulty = DIFFICULTY_MEDIUM,
        .endpoint = {
            .uri = "/query",
            .method = HTTP_POST,
            .handler = sqli_challenge_handler
        }
    },
    {
        .name = "XSS Attack",
        .description = "Learn about Cross-Site Scripting vulnerabilities",
        .difficulty = DIFFICULTY_MEDIUM,
        .endpoint = {
            .uri = "/message",
            .method = HTTP_POST,
            .handler = xss_challenge_handler
        }
    }
};

esp_err_t web_challenges_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server!");
        return ret;
    }

    // Register all challenge endpoints
    for (size_t i = 0; i < sizeof(challenges)/sizeof(challenges[0]); i++) {
        httpd_register_uri_handler(server, &challenges[i].endpoint);
    }

    ESP_LOGI(TAG, "Web challenges server started");
    return ESP_OK;
}

esp_err_t start_challenge(uint8_t challenge_id) {
    if (challenge_id >= sizeof(challenges)/sizeof(challenges[0])) {
        return ESP_ERR_INVALID_ARG;
    }

    challenges[challenge_id].status.start_time = esp_timer_get_time() / 1000000;
    challenges[challenge_id].status.attempts = 0;
    challenges[challenge_id].status.completed = false;

    ESP_LOGI(TAG, "Started challenge: %s", challenges[challenge_id].name);
    return ESP_OK;
}

challenge_status_t get_challenge_status(uint8_t challenge_id) {
    if (challenge_id >= sizeof(challenges)/sizeof(challenges[0])) {
        challenge_status_t empty_status = {0};
        return empty_status;
    }
    return challenges[challenge_id].status;
}