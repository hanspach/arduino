#include <declarations.h>
#include <server.h>
#include <dcf77.h>

#define DCF_PIN 2	         // Connection pin to DCF 77 device
#define DCF_INTERRUPT 0		 // Interrupt number associated with pin

DCF77 dcf = DCF77(15,DCF_INTERRUPT);
extern QueueHandle_t hQueue;
extern bool fallingEdge;              // defined in dcf
struct tm dt;
static char buffer[80] = "\0";
extern bool minStart;
void analysis(void*);


void setup() {
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  //setupDCF();
  //setupServer();

  dcf.Start();
}

void loop() {
  delay(1000);
  time_t t = dcf.getTime(); // Check if new DCF77 time is available
  if(t != 0)
  {
    Serial.printf("%d.%d.%d - %d:%d",day(t),month(t),
      year(t),hour(t),minute(t));
    
  }	
}

void loop() {
  static char buffer[80] = "\0";
  delay(1000);

  if(hQueue != NULL ) {
      if(xQueueReceive(hQueue, &dt, pdMS_TO_TICKS(10)) == pdPASS) {
        sprintf(buffer,"Datum: %2d.%2d.%4d  Uhrzeit: %2d:%2d",
          dt.tm_mday,dt.tm_mon,dt.tm_year,dt.tm_hour,dt.tm_min);
      }
  }

  runServer(buffer);
  delay(10);
  
  time_t t = DCF77::getTime(); // Check if new DCF77 time is available
  if(t != 0)
  {
    Serial.printf("%d.%d.%d - %d:%d\n",day(t),month(t),
      year(t),hour(t),minute(t));
    
  }	
}