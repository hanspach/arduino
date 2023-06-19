#include <Arduino.h>
#include <HardwareSerial.h>

#define UART 2
#define DATA_SIZE 2
#define TXD_PIN 17
#define RXD_PIN 16
#define USE_INTERNAL_PIN_LOOPBACK 1

HardwareSerial ser(UART);

void cbfReceive(void) {
  int bytes = ser.available();
  Serial.printf("%d: ",bytes);
  while(bytes--) {
    Serial.print((char)ser.read());
  }
  Serial.print(", ");
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  ser.begin(9600,SERIAL_8N1,RXD_PIN,TXD_PIN);
#if USE_INTERNAL_PIN_LOOPBACK
  uart_internal_loopback(UART, RXD_PIN);
#endif
  Serial.flush();
  ser.setRxFIFOFull(DATA_SIZE);
  ser.onReceive(cbfReceive);
}

void loop() {
 
}

