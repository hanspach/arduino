#include <Arduino.h>
#include "rcswitch.h"

// define static variables
 const RCSwitch::Protocol proto[] = {
  { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
};

RCSwitch::RCSwitch() {
  nRepeatTransmit = 10;
  protocol = proto[0];
}

void RCSwitch::enableTransmit(int transmitPin) {
  txPin = transmitPin;
  pinMode(txPin, OUTPUT);
}

void RCSwitch::send(const char* txt) {
  for(const char* p=txt; *p != '\0'; ++p) {
    send((unsigned long)*p, 8);
  }
}

/**
 * Transmit the first 'length' bits of the integer 'code'. The
 * bits are sent from MSB to LSB, i.e., first the bit at position length-1,
 * then the bit at position length-2, and so on, till finally the bit at position 0.
 */
void RCSwitch::send(unsigned long code, unsigned int length) {
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

