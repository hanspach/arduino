#include <blink.h>
#include <uisensor.h>

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
  if(!Serial) {
      Serial.begin(9600);
      while (!Serial) {
          delay(1);
      }
  }
  initIna();
 // initPWM();
}

void loop() {
  measureIna();
//  performPWM();
  delay(1000);
}