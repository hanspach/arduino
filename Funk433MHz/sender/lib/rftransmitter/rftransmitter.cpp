#include <Arduino.h>
#include "rftransmitter.h"

enum {
  MAX_PAYLOAD_SIZE = 80,
  MIN_PACKAGE_SIZE = 4,
  MAX_PACKAGE_SIZE = MAX_PAYLOAD_SIZE + MIN_PACKAGE_SIZE
};

static uint16_t crc_update(uint16_t crc, uint8_t data) {
#if defined(__AVR__)
return _crc_ccitt_update(crc, data);
#else
    // Source: http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__util__crc_1ga1c1d3ad875310cbc58000e24d981ad20.html
    data ^= crc & 0xFF;
    data ^= data << 4;

    return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4)
            ^ ((uint16_t)data << 3));
#endif
}

 RFTransmitter::RFTransmitter(byte outputPin, byte nodeId, unsigned int pulseLength,
        unsigned int backoffDelay, byte resendCount) : packageId(0), nodeId(nodeId), outputPin(outputPin),
        pulseLength(pulseLength), backoffDelay(backoffDelay), resendCount(resendCount) {

      pinMode(outputPin, OUTPUT);
      digitalWrite(outputPin, LOW);
      lineState = LOW;
}

void RFTransmitter::sendPackage(byte *data, byte len) {
  sendByte(0x00);    // Synchronize receiver
  sendByte(0x00);
  sendByte(0xE0);

  // Add from, id and crc to the message
  byte packageLen = len + 4;
  sendByteRed(packageLen);

  uint16_t crc = 0xffff;
  crc = crc_update(crc, packageLen);

  for (byte i = 0; i < len; ++i) {
    sendByteRed(data[i]);
    crc = crc_update(crc, data[i]);
  }

  sendByteRed(nodeId);
  crc = crc_update(crc, nodeId);

  sendByteRed(packageId);
  crc = crc_update(crc, packageId);

  sendByteRed(crc & 0xFF);
  sendByteRed(crc >> 8);

  digitalWrite(outputPin, LOW);
  lineState = LOW;
}

void RFTransmitter::resend(byte *data, byte len) {
  if (len > MAX_PAYLOAD_SIZE)
    len = MAX_PAYLOAD_SIZE;

  sendPackage(data, len);

  for (byte i = 0; i < resendCount; ++i) {
    delay(random(backoffDelay, backoffDelay << 1));
    sendPackage(data, len);
  }
}

void RFTransmitter::send(byte *data, byte len) {
  ++packageId;
  resend(data, len);
}

void RFTransmitter::send0() {
    lineState = !lineState;
    digitalWrite(outputPin, lineState);
    delayMicroseconds(pulseLength << 1);
}

void RFTransmitter::send1() {
    lineState = !lineState;
    digitalWrite(outputPin, lineState);
    delayMicroseconds(pulseLength);
    digitalWrite(outputPin, lineState);
    delayMicroseconds(pulseLength);
}

void RFTransmitter::send00() {
    send0();
    delayMicroseconds(pulseLength << 1);
}

void RFTransmitter::send01() {
    send1();
    delayMicroseconds(pulseLength << 1);
}

void RFTransmitter::send10() {
    send1();
    send0();
}

void RFTransmitter::send11() {
    send1();
    send1();
}

void RFTransmitter::sendByte(byte data) {
    byte i = 4;
    do {
    switch(data & 3) {
    case 0:
        send00();
        break;
    case 1:
        send01();
        break;
    case 2:
        send10();
        break;
    case 3:
        send11();
        break;
    }
    data >>= 2;
    } while(--i);
}

void RFTransmitter::sendByteRed(byte data) {
    sendByte(data);
    sendByte(data);
    sendByte(data);
}

void RFTransmitter::setBackoffDelay(unsigned int millies) { 
    backoffDelay = millies; 
}

void RFTransmitter::print(char *message) { 
    send((byte *)message, strlen(message)); 
}

void RFTransmitter::print(unsigned int value, byte base) {
    char printBuf[5];
    byte len = 0;

    for (byte i = sizeof(printBuf) - 1; i >= 0; --i, ++len) {
    printBuf[i] = "0123456789ABCDEF"[value % base];
    value /= base;
    }

    byte *data = (byte *)printBuf;
    while (*data == '0' && len > 1) {
    --len;
    ++data;
    }

    send(data, len);
}