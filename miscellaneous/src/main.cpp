#include <Arduino.h>
#include "blink.h"
#include "server.h"
#include "i2cScan.h"
#include "temperaturesensors.h"
#include "dcf_main.h"
#include "u8g2test.h"
#include "hc_sr501.h"

#ifndef LED_BUILTIN
    #define LED_BUILTIN 2
#endif
// STA_Webserver
// in the file 'server.cpp' adapt ssid & pwd to your router
// start Serial Monitor, run the programme
// copy the IP into the browser prompt e.g. http://192.168.0.23/
// switch LED on/off http://192.168.0.23/ledon http://192.168.0.23/ledoff

// runScanner
// finds i2c devices
void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(1); }
  
  //initBlinking();
  //setupServer();
  //initSensors();
  //dcfInitialising();
  //initU8g2();
  initMotionSensor(15, LED_BUILTIN);
}

void loop() {
  //runServer("Switch LED on/off via browser");
  //runScanner();
  //runSensors();
  //turnLedOnOff();
  //dcfRun();
  //runU8g2();
  runMotionSensor();
  //delay(1000);
}