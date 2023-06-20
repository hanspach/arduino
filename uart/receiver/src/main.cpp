#include <Arduino.h>
#include <HardwareSerial.h>

#define UART 2
#define DATA_SIZE 3
#define TXD_PIN 17
#define RXD_PIN 16

HardwareSerial ser(UART);

void cbfReceive(void) {
  int bytes = ser.available();
  Serial.printf("%d: ",bytes);
  while(bytes--) {
    char c = (char)ser.read();
    if(c) 
      Serial.print(c);
  }
  Serial.print(", ");
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  ser.begin(9600,SERIAL_8N1,RXD_PIN,TXD_PIN);
  ser.setRxFIFOFull(DATA_SIZE);
  ser.setRxTimeout(3);
  ser.onReceive(cbfReceive);
}

void loop() {
 
}

