#ifndef _RCSwitch_h
#define _RCSwitch_h
#include <Arduino.h>

class RCSwitch {
    /**
     * Description of a single pule, which consists of a high signal
     * whose duration is "high" times the base pulse length, followed
     * by a low signal lasting "low" times the base pulse length.
     * Thus, the pulse overall lasts (high+low)*pulseLength
     */
    struct HighLow {
        uint8_t high;
        uint8_t low;
    };

     /**
     * A "protocol" describes how zero and one bits are encoded into high/low
     * pulses.
     */
    struct Protocol {
        /** base pulse length in microseconds, e.g. 350 */
        uint16_t pulseLength;

        HighLow syncFactor;
        HighLow zero;
        HighLow one;
        bool invertedSignal;
    };
    void transmit(HighLow pulses);
    int txPin;
    Protocol protocol;
    int nRepeatTransmit;
public:
    RCSwitch();
    void enableTransmit(int nTransmitterPin);
    void send(unsigned long code, unsigned int length);
};
#endif