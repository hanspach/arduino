#ifndef _RCSwitch_h
#define _RCSwitch_h
#include <Arduino.h>
// Number of maximum high/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 67
#define BUFLEN 80

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
    void transmit(HighLow pulses);
    static bool IRAM_ATTR receiveProtocol(const int p, unsigned int changeCount);
    static void IRAM_ATTR handleInterrupt();
    Protocol protocol;
    uint8_t rxPin;
    uint8_t txPin;
    int nRepeatTransmit;

    static int nReceiveTolerance;
    static char* pBuf;
    volatile static unsigned long nReceivedValue;
    volatile static unsigned int nReceivedBitlength;
    volatile static unsigned int nReceivedDelay;
    volatile static unsigned int nReceivedProtocol;
    const static unsigned int nSeparationLimit;
    static portMUX_TYPE mux;
    static unsigned int timings[RCSWITCH_MAX_CHANGES];
public:
    RCSwitch();
    void enableTransmit(int nTransmitterPin);
    void enableReceive(int nReceiverPin);
    void disableTransmit();
    void disableReceive();
    bool available();
    void resetAvailable();
    void send(unsigned long code, unsigned int length = 24);
    void send(const char* txt);
    unsigned long getReceivedValue();
    char* getReceivedString();
    void resetString();
};
#endif