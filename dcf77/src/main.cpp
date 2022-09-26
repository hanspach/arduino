#include <declarations.h>
#include <server.h>

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  setupDCF();
  setupServer();
}

void loop() {
  runServer();
}