#include <Arduino.h>
#include "hc_sr501.h"

int MOTION_PIN;
int LED_PIN;
volatile int displayEnable = 1;

void IRAM_ATTR isrMotion() {
  static unsigned long prevTime = 0;
  unsigned long newTime = millis();
  
  displayEnable = digitalRead(MOTION_PIN);
  
}
 
void initMotionSensor(const int motionPin, const int ledPin) {
    MOTION_PIN = motionPin;
    LED_PIN = ledPin;
    pinMode(MOTION_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(motionPin), isrMotion, CHANGE);
}

void runMotionSensor() {
    digitalWrite(LED_PIN, displayEnable);
}
