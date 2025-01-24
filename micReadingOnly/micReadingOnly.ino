#include <Arduino.h>

const short MIC_PIN = 13; // the pin we connect the MIC to
int currentState; // what the new state is
int pastState = HIGH; // what the past state was
unsigned long previousMillis = 0; // how we tell when we last switched state
unsigned long shortInterval = 700; // how we differentiate between short + long, in milliseconds so this is .5 seconds
unsigned long longInterval = 2000;
const unsigned long debounceInterval = 50;  // Ignore rapid changes under 50ms

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
      Serial.println("DOT");
    } else if (timeDifference <= longInterval) {
      Serial.println("DASH");
    } else {
      Serial.println("IGNORE");
    }
    

    previousMillis = currentMillis;
  }
  pastState = currentState;
  
}
