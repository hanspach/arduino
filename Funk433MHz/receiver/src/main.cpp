#include <Arduino.h>
#include <rcswitch.h>

#define TX_PIN  17
#define RX_PIN  16

RCSwitch rxswitch = RCSwitch();

void setup() {

  delay(500);
  Serial.begin(9600);
  rxswitch.enableReceive(RX_PIN);
}

void loop() {
 char* res = rxswitch.getReceivedString();
 if(res != NULL) {
    Serial.printf("Es sind %s°C\n",res);
    rxswitch.resetString();
 }
}