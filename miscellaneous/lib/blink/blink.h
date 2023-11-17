#ifndef _HEADER_H
#define _HEADER_H
#include <Arduino.h>
#ifndef LED_BUILTIN
    #define LED_BUILTIN 2
#endif

void turnLEDon(int bit = LED_BUILTIN);
void turnLEDoff(int bit = LED_BUILTIN);
void turnLedOnOff();
#endif