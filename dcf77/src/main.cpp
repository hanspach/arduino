#include <declarations.h>
#include <server.h>

extern QueueHandle_t hQueue;
extern bool fallingEdge;              // defined in dcf
struct tm dt;
extern bool minStart;
void analysis(void*);

void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  setupDCF();
  setupServer();
}

void loop() {
  static char buffer[80] = "\0";

  if(hQueue != NULL ) {
      if(xQueueReceive(hQueue, &dt, pdMS_TO_TICKS(10)) == pdPASS) {
        sprintf(buffer,"Datum: %2d.%2d.%4d  Uhrzeit: %2d:%2d",
          dt.tm_mday,dt.tm_mon,dt.tm_year,dt.tm_hour,dt.tm_min);
      }
  }
  runServer(buffer);
  delay(10);
}