#ifndef _RCSwitch_h
#define _RCSwitch_h
#include <Arduino.h>
// Number of maximum high/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 67

class RCSwitch {
public:
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

private:
    static bool  receiveProtocol(const int p, unsigned int changeCount);
    static void  handleInterrupt();
    Protocol protocol;
   
    static int nReceiveTolerance;
    volatile static unsigned long receivedValue;
    static const unsigned int nSeparationLimit;
     /* 
     * timings[0] contains sync timing, followed by a number of bits
     */
    static unsigned int timings[RCSWITCH_MAX_CHANGES];
public:
    RCSwitch();
    void enableReceive(uint8_t rxPin);
    void disableReceive(uint8_t rxPin);
    bool available();
    void resetAvailable();
    unsigned long getReceivedValue();
};
#endif