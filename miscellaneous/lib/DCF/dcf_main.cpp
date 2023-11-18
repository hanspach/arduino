#include <Arduino.h>
#include "dcf77.h"
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#define DCF_PIN 15

void dcfInitialising() {
//   DCF::getInstance()->init(DCF_PIN, LED_BUILTIN);
#ifdef ARDUINO_ARCH_ESP32
    Serial.println("A esp32");
#endif
    dcfInit(digitalPinToInterrupt(DCF_PIN), LED_BUILTIN);
}

void dcfRun() {
     std::string msg;
    struct tm dt;

    if(dcfStateRequest(msg)) {
        if(dcfDateRequest(dt)) {
            Serial.printf("%d.%d.%d %d:%d",dt.tm_mday,dt.tm_mon,dt.tm_year,dt.tm_hour,dt.tm_min);
        }
        Serial.println(msg.c_str());
    }
}