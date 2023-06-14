#include <Arduino.h>
#include <HardwareSerial.h>
#define TXD_PIN 17
#define RXD_PIN 16

void setup() {
  Serial.begin(9600);
  if(!Serial) {
    while(1);
  } 

  Serial2.begin(9600,SERIAL_8N1,RXD_PIN, TXD_PIN);
  if(!Serial2) {
    Serial.println("Can't init Serial2");
  }
}

void loop() {
  static int count = 1; 

  Serial2.write(count);
  delay(1000);

}
