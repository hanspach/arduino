#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#define TXD_PIN 17
#define RXD_PIN 16

const uint8_t BUS = 4;
OneWire oneWire(BUS);
DallasTemperature sensor(&oneWire);

void setup() {
#ifdef _DEBUG_
  Serial.begin(9600);
  if(!Serial) {
    while(1);
  } 
#endif
  Serial2.begin(9600,SERIAL_8N1,RXD_PIN, TXD_PIN);
  if(!Serial2) {
#ifdef _DEBUG_
    Serial.println("Can't init Serial2");
#endif
    while(1);
  }
  sensor.begin();
  if(!sensor.isConnected(&BUS)) {
#ifdef _DEBUG_
    Serial.println("Can't init DS1820");
#endif
    while(1);
  }
}

void loop() {
  static int count = 1; 

  if(Serial2.availableForWrite()) {
    Serial2.write(count);
    Serial.printf("%d ",count);
    ++count;
  }
  delay(1000);

}
