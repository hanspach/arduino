#include <Arduino.h>
#include "rcswitch.h"

RCSwitch::RCSwitch() {
  txPin = -1;
  nRepeatTransmit = 10;
  static const Protocol proto[] = {
  { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
  { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
  { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
  { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
  { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
  { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 6 (HT6P20B)
  { 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false },    // protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
  { 200, {  3, 130}, {  7, 16 }, {  3,  16}, false},     // protocol 8 Conrad RS-200 RX
  { 200, { 130, 7 }, {  16, 7 }, { 16,  3 }, true},      // protocol 9 Conrad RS-200 TX
  { 365, { 18,  1 }, {  3,  1 }, {  1,  3 }, true },     // protocol 10 (1ByOne Doorbell)
  { 270, { 36,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 11 (HT12E)
  { 320, { 36,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 12 (SM5212)
};
  protocol = proto[0];
}

void RCSwitch::enableTransmit(int nTransmitterPin) {
  txPin = nTransmitterPin;
  pinMode(txPin, OUTPUT);
}

/**
 * Transmit the first 'length' bits of the integer 'code'. The
 * bits are sent from MSB to LSB, i.e., first the bit at position length-1,
 * then the bit at position length-2, and so on, till finally the bit at position 0.
 */
void RCSwitch::send(unsigned long code, unsigned int length) {
  if (txPin != -1) {
    for (int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++) {
        for (int i = length-1; i >= 0; i--) {
            if (code & (1L << i))
                transmit(protocol.one);
            else
                transmit(protocol.zero);
        }
        transmit(protocol.syncFactor);
    }
    // Disable transmit after sending (i.e., for inverted protocols)
    digitalWrite(txPin, LOW);    
  }
}

/**
 * Transmit a single high-low pulse.
 */
void RCSwitch::transmit(HighLow pulses) {
  uint8_t firstLogicLevel = protocol.invertedSignal ? LOW : HIGH;
  uint8_t secondLogicLevel= protocol.invertedSignal ? HIGH : LOW;
  
  digitalWrite(txPin, firstLogicLevel);
  delayMicroseconds(protocol.pulseLength * pulses.high);
  digitalWrite(txPin, secondLogicLevel);
  delayMicroseconds(protocol.pulseLength * pulses.low);
}
