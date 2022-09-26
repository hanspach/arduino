#ifndef _DECLARATIONS_H
#define _DECLARATIONS_H
#include <Arduino.h>

#ifndef LED_BUILTIN
    #define LED_BUILTIN 2
#endif
#define DCF_PIN 15
#define ESP32_DEBUG   // for dubug purpose

void dcfInit();
void prepare();
void evaluation();
#endif