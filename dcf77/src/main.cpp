#include <declarations.h>
<<<<<<< HEAD

extern bool fallingEdge;              // defined in dcf
unsigned long t1;
struct tm dt;
extern bool minStart;
void analysis(void*);
=======
#include <server.h>
>>>>>>> d1e7f6b8bdf2b16d9b13de80625e43a43c9cfad5

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  setupDCF();
  setupServer();
}

void loop() {
<<<<<<< HEAD
  static unsigned long t2;
  static bool status = false;
  static struct tm dtDCF;
  if(fallingEdge && status != fallingEdge) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));         // toggle LED
  }
  status = fallingEdge;
  t2 = millis();
  if((t2 - t1)  > 995) {
    minStart = true;
    analysis(NULL);
    minStart = false;
    if(++dt.tm_sec > 59) {
      dt.tm_sec = 0;
      if(++dt.tm_min > 59) {
        dt.tm_min = 0;
        if(++dt.tm_hour > 23) {
          dt.tm_hour = 0;
        } 
      }
    }
    t1 = t2;
  }
  delay(10);
=======
  runServer();
>>>>>>> d1e7f6b8bdf2b16d9b13de80625e43a43c9cfad5
}