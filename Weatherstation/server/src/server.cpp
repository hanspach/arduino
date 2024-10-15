#include <Arduino.h>
#include <bluefruit.h>
#include <WString.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include "AdvBLEAdvertising.h"

// #define _DEBUG_  // comment out in a real operation

void startAdv(int& grd, uint16_t& mv) {  
  BLEEddystoneTLM beacon;
  
  beacon.setVolt(mv);
  beacon.setTemp(grd);
  AdvBLEAdvertising adv(Bluefruit.Advertising);
  adv.setBeacon(beacon);

  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(5);                // 0 = Don't stop advertising after n seconds  

#ifdef _DEBUG_ 
  Serial.println(beacon.toString().c_str());
#endif
}

float getBatteryVoltage() {
  unsigned int adcCount = analogRead(PIN_VBAT);
  return ((510e3 + 1000e3) / 510e3) * 2.4 * adcCount / 4096;
}

Adafruit_MCP9808 sensor = Adafruit_MCP9808();

void setup() {
#ifdef _DEBUG_    
    Serial.begin(9600);
    while(!Serial);
#endif
  pinMode(PIN_VBAT, INPUT);       //Battery Voltage monitoring pin
  pinMode(VBAT_ENABLE, OUTPUT);   //Charge Current setting pin
  digitalWrite(VBAT_ENABLE, LOW);
  analogReadResolution(ADC_RESOLUTION);
  analogReference(AR_INTERNAL_2_4);  //Vref=2.4V

  sensor.begin(0x18);
  sensor.setResolution(1);
  Bluefruit.begin();
  Bluefruit.autoConnLed(false); // off Blue LED for lowest power consumption
  Bluefruit.setTxPower(8);    // 8=max power, Check bluefruit.h for supported values 
}

void loop() {
  sensor.wake();
  float fTemp = sensor.readTempC();
  int iTemp = (int)fTemp;
  uint16_t mv = (uint16_t)(getBatteryVoltage() * 1000);

  startAdv(iTemp, mv);
  delay(1000);
  sensor.shutdown();
  delay(150000);      // 2.5 min
}
