#ifndef RECEIVER_H
#define RECEIVER_H
#include <Arduino.h>

enum {
  MAX_PAYLOAD_SIZE = 80,
  MIN_PACKAGE_SIZE = 4,
  MAX_PACKAGE_SIZE = MAX_PAYLOAD_SIZE + MIN_PACKAGE_SIZE,
  MAX_SENDER_ID = 31
};

typedef unsigned char byte;

class RFReceiver {
    const byte inputPin;
    static unsigned int pulseLimit;
    static byte shiftByte;             // Input buffer and input state
    static byte errorCorBuf[3];
    static byte bitCount;
    static byte byteCount;
    static byte errorCorBufCount;
    static unsigned long lastTimestamp;
    static bool packageStarted;
    static volatile bool inputBufReady;
    static byte changeCount;
    static byte inputBuf[MAX_PACKAGE_SIZE];
    static byte inputBufLen;
    static uint16_t checksum;
    static byte prevPackageIds[MAX_SENDER_ID + 1]; // Used to filter out duplicate packages
    
    static byte recoverByte(const byte b1, const byte b2, const byte b3);
    static uint16_t crc_update(uint16_t crc, uint8_t data);
    byte RFReceiver::recvDataRaw(byte * data);
    byte recvDataRaw(byte * data);
    static void decodeByte(byte inputByte);
    static void handleInterrupt();
public:
    RFReceiver(byte inputPin, unsigned int pulseLength = 100);
    byte recvPackage(byte * data, byte *pSenderId = 0, byte *pPackageId = 0);
    
    void begin();
    void stop();
    bool ready() const; 
};
#endif