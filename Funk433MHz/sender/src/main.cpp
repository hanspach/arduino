#include <Arduino.h>
#include <rcswitch.h>
#include "Adafruit_MCP9808.h"

RCSwitch sender = RCSwitch();
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

void setup() {
  Serial.begin(9600);
  delay(500);
  tempsensor.begin(0x18);                           // default address
  tempsensor.setResolution(1);                      // 
  sender.enableTransmit(17);
  
}

void loop() {
  float c = tempsensor.readTempC();
  int i = (int)c;
  Serial.printf("Temperatur: %d°C\n",i);
  sender.send(i, 24);
  delay(2000);
}
