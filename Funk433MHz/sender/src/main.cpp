#include <Arduino.h>
#include <rcswitch.h>

RCSwitch sender = RCSwitch();

void setup() {
  sender.enableTransmit(17);
  //txs.begin(2400,SERIAL_8N1,16,17);
}

void loop() {
  for(int i=1; i < 10; i++) {
    sender.send(i, 24);
    delay(100);
  }
  delay(5000);
}
