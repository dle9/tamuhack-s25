#include <Arduino.h>

const short MIC_PIN = 13;
const short BLUE_PIN = 27;
const short GREEN_PIN = 26;
const short RED_PIN = 25;
int currentState;
int pastState = HIGH;
unsigned long previousMillis = 0; // how we tell when we last switched state
unsigned long shortInterval = 700; // how we differentiate between short + long, in milliseconds so this is .5 seconds
unsigned long longInterval = 2000;
const unsigned long debounceInterval = 50;  // Ignore rapid changes under 50ms

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
}

void loop() {
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
