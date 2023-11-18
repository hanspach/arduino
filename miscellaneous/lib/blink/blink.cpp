#include <blink.h>

void initBlinking(const int PORT) {
  pinMode(PORT, OUTPUT);
}

void turnLEDon(int port) {
    digitalWrite(port, HIGH);
}

void turnLEDoff(int port) {
    digitalWrite(port, LOW);
}

void turnLedOnOff() {
  delay(1000);
  turnLEDon();
  Serial.println("Die LED ist an.");
  delay(1000);
  turnLEDoff();
  Serial.println("Die LED ist aus.");
}