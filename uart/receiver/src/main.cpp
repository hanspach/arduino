#include <Arduino.h>
#include <HardwareSerial.h>
#define TXD_PIN 17
#define RXD_PIN 16
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
HardwareSerial ser(2);


void setup() {
 ser.begin(9600,SERIAL_8N1,RXD_PIN,TXD_PIN);
 pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if(ser.available()) {
    int res = ser.read();
    if(res % 2)
        digitalWrite(LED_BUILTIN, HIGH);
    else
        digitalWrite(LED_BUILTIN, LOW);
  }
}

