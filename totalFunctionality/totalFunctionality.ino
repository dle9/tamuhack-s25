#include <esp_now.h>
#include <WiFi.h>


const short MIC_PIN = 13;
const short BLUE_PIN = 27;
const short GREEN_PIN = 26;
const short RED_PIN = 25;

const unsigned shortInterval = 700;
const unsigned longInterval = 2000;
const unsigned short debounceInterval = 50;
unsigned long long previousMillis = 0;
int currentState;
int pastState = 0;
byte letter = 0; // letter will be 1 when short, 2 when long, 3 when pause



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
//
unsigned long lastSendTime = 0;  // Last time data was sent
const unsigned long sendInterval = 2000;  // 2 seconds between sends

#define MAX_PACKET_SIZE 250
uint8_t dataBuffer[MAX_PACKET_SIZE];
size_t bufferIndex = 0;
unsigned long lastCollectionTime = 0;
const unsigned long collectionInterval = 100;  // Collect data every 100ms


void setup() {
  Serial.begin(115200);

  pinMode(MIC_PIN, INPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);

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

  // collect and display audio sensor data

  unsigned long currentMillis = millis();
  unsigned long timeDifference = currentMillis - previousMillis;

  if (timeDifference <= shortInterval) {
    analogWrite(BLUE_PIN, 0);
    analogWrite(GREEN_PIN, 255);
    analogWrite(RED_PIN, 0);
  } else if (timeDifference <= longInterval) {
    analogWrite(BLUE_PIN, 255);
    analogWrite(GREEN_PIN, 0);
    analogWrite(RED_PIN, 0);
  } else {
    analogWrite(BLUE_PIN, 0);
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 0);
  }
  
  currentState = digitalRead(MIC_PIN);
  if (currentState != pastState && (currentMillis - previousMillis > debounceInterval)) {

    if (timeDifference <= shortInterval) {
      //Serial.println("DOT");
      letter = 1;
    } else if (timeDifference <= longInterval) {
      //Serial.println("DASH");
      letter = 2;
    } else {
      //Serial.println("IGNORE");
      letter = 3;
    }

    if (millis() - lastCollectionTime >= collectionInterval) {
      if (bufferIndex < MAX_PACKET_SIZE - 1) {
        dataBuffer[bufferIndex++] = letter;
        //Serial.println(letter);
      } else {
        esp_err_t result = esp_now_send(broadcastAddress, dataBuffer, bufferIndex);
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        } else {
          Serial.println("Error sending buffer");
        }
        bufferIndex = 0;  // Reset buffer after sending
      }
      lastCollectionTime = millis();
    }

    previousMillis = currentMillis;
  }
  pastState = currentState;


// Collect data at defined intervals

  

  // Send collected data every 2 seconds if needed
  if (millis() - lastSendTime >= sendInterval && bufferIndex > 0) {
    esp_err_t result = esp_now_send(broadcastAddress, dataBuffer, bufferIndex);
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
    }
    bufferIndex = 0;  // Reset buffer
    lastSendTime = millis();
  }
}