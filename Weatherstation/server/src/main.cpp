#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#include <dcf77.h>
#define TXD_PIN 17
#define RXD_PIN 16
#define DCF_PIN 12
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#define _DEBUG_

const uint8_t BUS = 4;
const char* DAYS[] = {"","Mo","Di","Mi","Do","Fr","Sa","So"};
char buffer[36] = "\0";
OneWire oneWire(BUS);
DallasTemperature sensor(&oneWire);

void setup() {
#ifdef _DEBUG_
  Serial.begin(9600);
  if(!Serial) {
    while(1);
  } 
#endif
  
  DCF77::Start(DCF_PIN);
  //Serial2.begin(9600,SERIAL_8N1,RXD_PIN, TXD_PIN);
  if(!Serial2) {
#ifdef _DEBUG_
    Serial.println("Can't init Serial2");
#endif
    while(1);
  }
  sensor.begin();
}

void loop() {
  sensor.requestTemperatures();
  float celsius = sensor.getTempCByIndex(0);
  int c = (int)celsius;
  uint8_t wota = 1;
  time_t t = DCF77::getTime();
  if(t) {
    tm dt = toTmStruct(t);
    if(dt.tm_wday > 0 && dt.tm_wday <= 7) 
      wota = dt.tm_wday;
    sprintf(buffer,"%s, %d.%d.%d %d:%d-%d",DAYS[wota],dt.tm_mday,dt.tm_mon,dt.tm_year,dt.tm_hour,dt.tm_min,c);
    
  } else {
    sprintf(buffer,"2%d",c);
  }
  Serial.printf("%s ",buffer);
  /*
  if(Serial2.availableForWrite()) {
    Serial2.write(buffer,strlen(buffer));
  } */
  delay(5000);
}
