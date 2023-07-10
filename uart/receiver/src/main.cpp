#include <Arduino.h>
#include <HardwareSerial.h>

#define DATA_SIZE 24
#define TXD_PIN 17
#define RXD_PIN 16

void cbfReceive(void) {
  int bytes = Serial2.available();
  Serial.printf("%d: ",bytes);
  while(bytes--) {
    char c = (char)Serial2.read();
    if(c) 
      Serial.print(c);
  }
  Serial.print(", ");
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  Serial2.begin(9600,SERIAL_8N1,RXD_PIN,TXD_PIN);
  Serial2.setRxFIFOFull(DATA_SIZE);
  Serial2.onReceive(cbfReceive);
}

void loop() {
 
}

