#include <esp_now.h>
#include <WiFi.h>


const short MIC_PIN = 13; // the pin we connect the MIC to
int currentState; // what the new state is
int pastState = 0; // what the past state was
unsigned long previousMillis = 0; // how we tell when we last switched state
unsigned long shortInterval = 500; // how we differentiate between short + long, in milliseconds so this is .5 seconds


uint8_t broadcastAddress[] = {0x03, 0xb4, 0x16, 0x72, 0x36, 0x9c}; // MAC address of other esp32

esp_now_peer_info_t peerInfo;
String success;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}
uint8_t theirData;
uint8_t incomingData;

void OnDataRecv(const uint8_t * mac, const uint8_t theirData, int len) {
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingData = theirData;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}



void loop() {
  uint8_t myData = 8;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, 8);

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
  delay(2000); // how long we wait between loops, in ms
}