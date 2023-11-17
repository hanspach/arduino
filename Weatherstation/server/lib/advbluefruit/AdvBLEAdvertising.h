#ifndef _AdvAdvertising_
#define _AdvAdvertising_
#include <Arduino.h>
#include <bluefruit.h>
#include "BLEEddystoneTLM.h"

class AdvBLEAdvertising: public BLEAdvertising {
public:
    AdvBLEAdvertising();
    AdvBLEAdvertising(BLEAdvertising&);
    bool setBeacon(BLEEddystoneTLM&);
};
#endif