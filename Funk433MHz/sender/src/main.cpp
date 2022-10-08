#include <Arduino.h>
#include <HardwareSerial.h>

HardwareSerial txs(1);

void setup() {
  txs.begin(2400,SERIAL_8N1,16,15);
}

void loop() {
 const char *msg = "Hello World!";
  txs.flush();
  txs.println(msg);
  delay(1000);
}
