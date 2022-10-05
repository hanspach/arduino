#ifndef _HEADER_H
#define _HEADER_H
#include <Arduino.h>
#ifndef LED_BUILTIN
    #define LED_BUILTIN 2
#endif

void turnLEDon(int);
void turnLEDoff(int);
#endif