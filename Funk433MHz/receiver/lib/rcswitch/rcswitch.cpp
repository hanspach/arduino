#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "rcswitch.h"

/* Format for protocol definitions:
 * {pulselength, Sync bit, "0" bit, "1" bit, invertedSignal}
 * 
 * pulselength: pulse length in microseconds, e.g. 350
 * Sync bit: {1, 31} means 1 high pulse and 31 low pulses
 *     (perceived as a 31*pulselength long pulse, total length of sync bit is
 *     32*pulselength microseconds), i.e:
 *      _
 *     | |_______________________________ (don't count the vertical bars)
 * "0" bit: waveform for a data bit of value "0", {1, 3} means 1 high pulse
 *     and 3 low pulses, total length (1+3)*pulselength, i.e:
 *      _
 *     | |___
 * "1" bit: waveform for a data bit of value "1", e.g. {3,1}:
 *      ___
 *     |   |_
 *
 * These are combined to form Tri-State bits when sending or receiving codes.
 */

 const static DRAM_ATTR RCSwitch::Protocol proto[] = {
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

enum {
   numProto = sizeof(proto) / sizeof(proto[0])
};

/* helper function for the receiveProtocol method */
static unsigned int diff(int A, int B) {
  return abs(A - B);
}

// definition for static variables
volatile unsigned long RCSwitch::nReceivedValue = 0;
volatile unsigned int RCSwitch::nReceivedBitlength = 0;
volatile unsigned int RCSwitch::nReceivedDelay = 0;
volatile unsigned int RCSwitch::nReceivedProtocol = 0;
portMUX_TYPE RCSwitch::mux = portMUX_INITIALIZER_UNLOCKED;
int RCSwitch::nReceiveTolerance = 60;
unsigned int RCSwitch::timings[RCSWITCH_MAX_CHANGES];
char* RCSwitch::pBuf = NULL;
const unsigned int DRAM_ATTR RCSwitch::nSeparationLimit = 4300;
// separationLimit: minimum microseconds between received codes, closer codes are ignored.
// according to discussion on issue #14 it might be more suitable to set the separation
// limit to the same time as the 'low' part of the sync signal for the current protocol.

RCSwitch::RCSwitch() {
  nRepeatTransmit = 10;
  protocol = proto[0];
  nReceiveTolerance = 60;
  nRepeatTransmit = 10;
}

void RCSwitch::enableTransmit(int transmitPin) {
  txPin = transmitPin;
  pinMode(txPin, OUTPUT);
}

void RCSwitch::disableTransmit() {
  txPin = -1;
}

void RCSwitch::enableReceive(int nReceiverPin) {
  rxPin = nReceiverPin;
  pinMode(rxPin, INPUT);
  nReceivedValue = 0;
  attachInterrupt(rxPin,RCSwitch::handleInterrupt,CHANGE);
}
    
void RCSwitch::disableReceive() {
  detachInterrupt(rxPin);
}

bool RCSwitch::available() {
  return RCSwitch::nReceivedValue != 0;
}

void RCSwitch::resetAvailable() {
  RCSwitch::nReceivedValue = 0;
}

unsigned long RCSwitch::getReceivedValue() {
  return RCSwitch::nReceivedValue;
}

char* RCSwitch::getReceivedString() {
   return pBuf;
}
    
void RCSwitch::resetString() {
  if(pBuf != NULL) {
    portENTER_CRITICAL(&mux);
    free(pBuf); 
    pBuf = NULL;
    portEXIT_CRITICAL(&mux);
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

void RCSwitch::send(const char* txt) {
  const char* p = txt;
  do {
    send(*p);
    delay(10);
  } while(*p++ != 0);
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

void IRAM_ATTR RCSwitch::handleInterrupt() {
  static unsigned int changeCount = 0;
  static unsigned long lastTime = 0;
  static unsigned int repeatCount = 0;

  const long time = micros();
  const unsigned int duration = time - lastTime;
  if (duration > RCSwitch::nSeparationLimit) {
    // A long stretch without signal level change occurred. This could
    // be the gap between two transmission.
    if ((repeatCount==0) || (diff(duration, RCSwitch::timings[0]) < 200)) {
      // This long signal is close in length to the long signal which
      // started the previously recorded timings; this suggests that
      // it may indeed by a a gap between two transmissions (we assume
      // here that a sender will send the signal multiple times,
      // with roughly the same gap between them).
      repeatCount++;
      if (repeatCount == 2) {
        for(unsigned int i = 1; i <= numProto; i++) {
          if (RCSwitch::receiveProtocol(i, changeCount)) {
            // receive succeeded for protocol i
            break;
          }
        }
        repeatCount = 0;
      }
    }
    changeCount = 0;
  }
 
  // detect overflow
  if (changeCount >= RCSWITCH_MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }

  RCSwitch::timings[changeCount++] = duration;
  lastTime = time;  
}

bool IRAM_ATTR RCSwitch::receiveProtocol(const int p, unsigned int changeCount) {
    const Protocol &pro = proto[p-1];
    static uint8_t pos = 0;
    static uint8_t cnt = 0;
    unsigned long code = 0;
    static char buffer[BUFLEN] = "\0";
    //Assuming the longer pulse length is the pulse captured in timings[0]
    const unsigned int syncLengthInPulses =  ((pro.syncFactor.low) > (pro.syncFactor.high)) ? (pro.syncFactor.low) : (pro.syncFactor.high);
    const unsigned int delay = RCSwitch::timings[0] / syncLengthInPulses;
    const unsigned int delayTolerance = delay * RCSwitch::nReceiveTolerance / 100;
    
    /* For protocols that start low, the sync period looks like
     *               _________
     * _____________|         |XXXXXXXXXXXX|
     *
     * |--1st dur--|-2nd dur-|-Start data-|
     *
     * The 3rd saved duration starts the data.
     *
     * For protocols that start high, the sync period looks like
     *
     *  ______________
     * |              |____________|XXXXXXXXXXXXX|
     *
     * |-filtered out-|--1st dur--|--Start data--|
     *
     * The 2nd saved duration starts the data
     */
    const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

    for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
        code <<= 1;
        if (diff(RCSwitch::timings[i], delay * pro.zero.high) < delayTolerance &&
            diff(RCSwitch::timings[i + 1], delay * pro.zero.low) < delayTolerance) {
            // zero
        } else if (diff(RCSwitch::timings[i], delay * pro.one.high) < delayTolerance &&
                   diff(RCSwitch::timings[i + 1], delay * pro.one.low) < delayTolerance) {
            // one
            code |= 1;
        } else {
            // Failed
            return false;
        }
    }

    if (changeCount > 7) {    // ignore very short transmissions: no device sends them, so this must be noise
        if(cnt++ == 3) {
          buffer[pos++] = (char)code;
          cnt = 0;
          if(pos >= BUFLEN-2) 
            pos = 0;
          if(code == 0) { 
            portENTER_CRITICAL(&mux);
            pBuf = (char*)malloc(strlen(buffer)+1); 
            strcpy(pBuf,buffer);
            portEXIT_CRITICAL(&mux);
            pos = 0;
          }
        }
        RCSwitch::nReceivedValue = code;
        RCSwitch::nReceivedBitlength = (changeCount - 1) / 2;
        RCSwitch::nReceivedDelay = delay;
        RCSwitch::nReceivedProtocol = p;
        return true;
    }

    return false;
}

