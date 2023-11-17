#include "AdvBLEAdvertising.h"

AdvBLEAdvertising::AdvBLEAdvertising() : BLEAdvertising() {
}

AdvBLEAdvertising::AdvBLEAdvertising(BLEAdvertising& adv) {
}

bool AdvBLEAdvertising::setBeacon(BLEEddystoneTLM &eddy) {
  return eddy.start();
}