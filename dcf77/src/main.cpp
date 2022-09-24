#include <header.h>
#include <declarations.h>

extern bool fallingEdge;              // defined in dcf
unsigned long t1;
struct tm dt;

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  dcfInit();
  t1 = millis();
}

void loop() {
  static unsigned long t2;
  static bool status = false;
  static struct tm dtDCF;
  if(fallingEdge && status != fallingEdge) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));         // toggle LED
  }
  status = fallingEdge;
  t2 = millis();
  if((t2 - t1)  > 995) {
    if(++dt.tm_sec > 59) {
      dt.tm_sec = 0;
      if(++dt.tm_min > 59) {
        dt.tm_min = 0;
        if(++dt.tm_hour > 23) {
          dt.tm_hour = 0;
        } 
      }
    }
}