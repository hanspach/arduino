#include <Arduino.h>
#include "server.h"
#include "i2cScan.h"


// STA_Webserver
// in the file 'server.cpp' adapt ssid & pwd to your router
// start Serial Monitor, run the programme
// copy the IP into the browser prompt e.g. http://192.168.0.23/
// switch LED on/off http://192.168.0.23/ledon http://192.168.0.23/ledoff


void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(1); }
  
  //setupServer();
}

void loop() {
  ///runServer("Switch LED on/off via browser");
  runScanner();
  delay(5000);
}