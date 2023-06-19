#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#define TXD_PIN 17
#define RXD_PIN 16
#define _DEBUG_

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
}

void loop() {
 char buffer[36] = "\0";

 if(Serial2.availableForWrite()) {
    sensor.requestTemperatures();
    float celsius = sensor.getTempCByIndex(0);
    int c = (int)celsius;
    sprintf(buffer,"2%d",c);
    Serial2.write(buffer,strlen(buffer));
#ifdef _DEBUG_
    Serial.printf("T:%d °C ",c);
#endif
  }
  delay(1000);
}
