#include <Arduino.h>

const short MIC_PIN = 13; // the pin we connect the MIC to
int currentState; // what the new state is
int pastState = HIGH; // what the past state was
unsigned long previousMillis = 0; // how we tell when we last switched state
unsigned long shortInterval = 500; // how we differentiate between short + long, in milliseconds so this is .5 seconds
const unsigned long debounceInterval = 50;  // Ignore rapid changes under 50ms
bool changedRecently = 1;

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  currentState = digitalRead(MIC_PIN);
  if (currentState != pastState && (currentMillis - previousMillis > debounceInterval)) {
    unsigned long timeDifference = currentMillis - previousMillis;

    if (timeDifference <= shortInterval) {
      changedRecently = 1;
      Serial.println("Short change detected");
    } else {
      changedRecently = 0;
      Serial.println("Long change detected");
    }

    previousMillis = currentMillis;
  }
  pastState = currentState;
  
}
