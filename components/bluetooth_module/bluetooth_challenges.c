// components/bluetooth_module/bluetooth_challenges.c
#include "bluetooth_challenges.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "bluetooth_challenges";

// Current active challenge
static bluetooth_challenge_type_t active_challenge = -1;
static TaskHandle_t challenge_task_handle = NULL;

// Queue for BLE events
static QueueHandle_t ble_evt_queue = NULL;

// GATT profile for vulnerable service demonstration
static uint16_t vulnerable_service_handle;
static uint16_t vulnerable_char_handle;

#define VULNERABLE_SERVICE_UUID      0xFF00
#define VULNERABLE_CHARACTERISTIC_UUID 0xFF01

// Simulated device database for spoofing detection
typedef struct {
    esp_bd_addr_t addr;
    char name[32];
    int8_t rssi;
    uint32_t first_seen;
    uint32_t last_seen;
} known_device_t;

#define MAX_KNOWN_DEVICES 10
static known_device_t known_devices[MAX_KNOWN_DEVICES];
static int num_known_devices = 0;

// GAP event handler
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                // Process scan results based on active challenge
                switch (active_challenge) {
                    case BT_CHALLENGE_SCANNING:
                        ESP_LOGI(TAG, "Found device: " ESP_BD_ADDR_STR, 
                                ESP_BD_ADDR_HEX(param->scan_rst.bda));
                        ESP_LOGI(TAG, "RSSI: %d", param->scan_rst.rssi);
                        break;
                        
                    case BT_CHALLENGE_SNIFFING:
                        // Analyze advertisement data
                        if (param->scan_rst.adv_data_len > 0) {
                            ESP_LOGI(TAG, "Advertisement data:");
                            esp_log_buffer_hex(TAG, param->scan_rst.ble_adv, 
                                             param->scan_rst.adv_data_len);
                        }
                        break;
                        
                    case BT_CHALLENGE_SPOOFING:
                        // Check for suspicious device patterns
                        for (int i = 0; i < num_known_devices; i++) {
                            if (memcmp(known_devices[i].addr, param->scan_rst.bda, 
                                     ESP_BD_ADDR_LEN) == 0) {
                                // Check for suspicious RSSI changes
                                if (abs(known_devices[i].rssi - param->scan_rst.rssi) > 20) {
                                    ESP_LOGW(TAG, "Suspicious RSSI change detected!");
                                }
                                break;
                            }
                        }
                        break;
                }
            }
            break;
            
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            if (active_challenge == BT_CHALLENGE_PAIRING) {
                ESP_LOGI(TAG, "Pairing complete. Security level: %d", 
                         param->ble_security.auth_cmpl.auth_mode);
            }
            break;
    }
}

// GATT event handler
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, 
                              esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "GATT connection established");
            if (active_challenge == BT_CHALLENGE_MAN_IN_MIDDLE) {
                // Demonstrate MITM vulnerability
                ESP_LOGI(TAG, "Connection without authentication detected!");
            }
            break;
            
        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == vulnerable_char_handle) {
                ESP_LOGI(TAG, "Write to vulnerable characteristic:");
                esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            }
            break;
    }
}

// Task to handle scanning challenge
static void scanning_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting BLE Scanning Challenge");
    
    // Configure scan parameters
    esp_ble_scan_params_t scan_params = {
        .scan_type = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,
        .scan_window = 0x30,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
    };
    
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&scan_params));
    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(0));
    
    while (active_challenge == BT_CHALLENGE_SCANNING) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    esp_ble_gap_stop_scanning();
    vTaskDelete(NULL);
}

// Task to handle pairing challenge
static void pairing_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting BLE Pairing Challenge");
    
    // Set up different security levels for demonstration
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, 
                                                  &auth_req, sizeof(auth_req)));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, 
                                                  &iocap, sizeof(iocap)));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, 
                                                  &key_size, sizeof(key_size)));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, 
                                                  &init_key, sizeof(init_key)));
    ESP_ERROR_CHECK(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, 
                                                  &rsp_key, sizeof(rsp_key)));
    
    while (active_challenge == BT_CHALLENGE_PAIRING) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    vTaskDelete(NULL);
}

esp_err_t bluetooth_challenges_init(void) {
    ESP_LOGI(TAG, "Initializing Bluetooth security challenges");
    
    // Initialize Bluetooth
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    
    // Register callbacks
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    
    // Create event queue
    ble_evt_queue = xQueueCreate(10, sizeof(esp_gap_ble_cb_event_t));
    
    return ESP_OK;
}

esp_err_t start_bluetooth_challenge(bluetooth_challenge_type_t type) {
    if (active_challenge != -1) {
        ESP_LOGE(TAG, "Challenge already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    active_challenge = type;
    
    switch (type) {
        case BT_CHALLENGE_SCANNING:
            xTaskCreate(scanning_task, "scanning_task", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        case BT_CHALLENGE_PAIRING:
            xTaskCreate(pairing_task, "pairing_task", 4096, NULL, 5, &challenge_task_handle);
            break;
            
        default:
            ESP_LOGE(TAG, "Unknown challenge type");
            return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Started Bluetooth challenge type %d", type);
    return ESP_OK;
}

esp_err_t stop_bluetooth_challenge(void) {
    if (active_challenge == -1) {
        return ESP_OK;
    }
    
    active_challenge = -1;
    vTaskDelay(pdMS_TO_TICKS(100));  // Give time for task cleanup
    
    if (challenge_task_handle != NULL) {
        vTaskDelete(challenge_task_handle);
        challenge_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "Stopped Bluetooth challenge");
    return ESP_OK;
}