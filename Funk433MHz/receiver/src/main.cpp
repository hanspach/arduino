#include <Arduino.h>
#include <HardwareSerial.h>
#include <rcswitch.h>
#include "rfreceiver.h"

#define TX_PIN  17
#define RX_PIN  16
#define BUF_SIZE 1024


HardwareSerial rxs = HardwareSerial(1);
RCSwitch rxswitch = RCSwitch();
RFReceiver receiver = RFReceiver(RX_PIN);

void setup() {
  delay(500);
  Serial.begin(9600);
}

void loop() {
  static char msg[MAX_PACKAGE_SIZE];
  byte senderId = 0;
  byte packageId = 0;

  rxswitch.enableReceive(RX_PIN);
  if(rxswitch.available()) {
      Serial.printf("RCSwitch-Value: %d\n",rxswitch.getReceivedValue());
      rxswitch.resetAvailable();
  }
  rxswitch.disableReceive(RX_PIN);
  delay(50);
/*
  receiver.begin();
  byte len = receiver.recvPackage((byte *)msg, &senderId, &packageId);
  Serial.printf("RFReceiver-Value: %s\n",msg);
  receiver.stop();
 
  rxs.begin(2400,SERIAL_8N1,RX_PIN,TX_PIN);
  delay(100);
  if(rxs.available()) {
    String s = rxs.readStringUntil('\n');
    Serial.printf("Serial-Value:%s\n",s.c_str());
  }
  rxs.end();
  */
  delay(2000);
}