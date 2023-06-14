#include <Arduino.h>
#include <rcswitch.h>
#include "Adafruit_MCP9808.h"

#define TX_PIN  17
#define RX_PIN  16

RCSwitch sender = RCSwitch();
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

void setup() {
  Serial.begin(9600);
  delay(500);
  tempsensor.begin(0x18);                           // default address
  tempsensor.setResolution(1);                      // 
  sender.enableTransmit(TX_PIN);
}

void loop() {
  static char buffer[32] = "\0";
  float c = tempsensor.readTempC();
  int i = (int)c;
  sprintf(buffer,"%d",i);
  Serial.printf("Es sind %s°C\n",buffer);
  sender.send(buffer);
  delay(3000);
}
