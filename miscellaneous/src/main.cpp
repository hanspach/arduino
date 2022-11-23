#include <blink.h>

int status;

void turnLedOnOff() {
  delay(1000);
  turnLEDon();
  status = 1;
  Serial.println("Die LED ist an.");
  delay(1000);
  turnLEDoff();
  status = 0;
  Serial.println("Die LED ist aus.");
}

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  turnLedOnOff();
}