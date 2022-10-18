#ifndef RFTRANSMITTER_H_
#define RFTRANSMITTER_H_
#include <Arduino.h>

class RFTransmitter {
    byte packageId;

    const byte nodeId;
    const byte outputPin;
    const unsigned int pulseLength;     // Pulse lenght in microseconds
    unsigned int backoffDelay;          // Backoff period for repeated sends in milliseconds
    byte resendCount;                   // How often a reliable package is resent
    byte lineState;
    
    void send0();
    void send1();
    void send00();
    void send01();
    void send10();
    void send11();
    void sendByte(byte data);
    void sendByteRed(byte data);
    void sendPackage(byte *data, byte len);
public:
     RFTransmitter(byte outputPin, byte nodeId = 0, unsigned int pulseLength = 100,
        unsigned int backoffDelay = 100, byte resendCount = 1);
    void setBackoffDelay(unsigned int millies);
    void send(byte *data, byte len);
    void resend(byte *data, byte len);
    void print(char *message);
    void print(unsigned int value, byte base = DEC);
};
#endif