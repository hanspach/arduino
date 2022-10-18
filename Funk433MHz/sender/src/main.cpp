#include <Arduino.h>
#include <rcswitch.h>
#include "rftransmitter.h"
#include "Adafruit_MCP9808.h"

#define TX_PIN  17
#define RX_PIN  16
#define BUF_SIZE 1024

HardwareSerial txs = HardwareSerial(1);
RCSwitch sender = RCSwitch();
RFTransmitter trans = RFTransmitter(TX_PIN);
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

void setup() {
  Serial.begin(9600);
  delay(500);
  tempsensor.begin(0x18);                           // default address
  tempsensor.setResolution(1);                      // 
  sender.enableTransmit(17);
  
}

void loop() {
  static char buffer[BUF_SIZE] = "\0";
  float c = tempsensor.readTempC();
  int i = (int)c;
  sprintf(buffer,"Es sind %d°C\n",i);
  Serial.printf("RCSwitch sends: %d°C\n",i);
  sender.send(i, 24);
  delay(100);

  trans.send((byte*)buffer,strlen(buffer)+1);
  Serial.printf("RFTransmitter sends: %d°C\n",i);
  delay(100);
  
  txs.begin(2400,SERIAL_8N1,RX_PIN,TX_PIN);
  delay(100);
  txs.println(buffer);
  Serial.printf("Serial sends:%s\n",buffer);
  txs.end();
  delay(2000);
}
