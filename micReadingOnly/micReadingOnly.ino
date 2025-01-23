#include <Arduino.h>

const short MIC_PIN = 13; // the pin we connect the MIC to
int currentState; // what the new state is
int pastState = HIGH; // what the past state was
unsigned long previousMillis = 0; // how we tell when we last switched state
unsigned long shortInterval = 500; // how we differentiate between short + long, in milliseconds so this is .5 seconds
bool changedRecently = 1;

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  currentState = digitalRead(MIC_PIN);

  if (currentState == HIGH && pastState == LOW) {
    if (currentMillis - previousMillis <= shortInterval) {
      changedRecently = 1;
      Serial.println("changed recently");
    } else {
      changedRecently = 0;
      Serial.println("didn't change recently");
    }
    previousMillis = currentMillis;
  }
  else if (currentState == LOW && pastState == HIGH) {
    if (currentMillis - previousMillis <= shortInterval) {
      changedRecently = 1;
      Serial.println("changed recently");
    } else {
      changedRecently = 0;
      Serial.println("didn't change recently");
    }
    previousMillis = currentMillis;
  }
  pastState = currentState;
  
}
