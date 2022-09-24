#include <Arduino.h>
#include <RFTransmitter.h>

#define NODE_ID          1
#define OUTPUT_PIN       2
 unsigned long counter = 0;
RFTransmitter transmitter(OUTPUT_PIN, NODE_ID);

void setup() {
  
}

void loop() {
 const char *msg = "Hello World!";
  transmitter.send((byte *)msg, strlen(msg) + 1);

  delay(5000);

  transmitter.resend((byte *)msg, strlen(msg) + 1);
}
