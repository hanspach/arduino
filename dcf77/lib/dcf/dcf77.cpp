#include <declarations.h>

uint8_t bit = 0;
uint8_t  frame[60];
struct tm dtDCF;
bool minStart = false;
bool validDCF = false;
volatile bool fallingEdge = false; 
unsigned long t1;
QueueHandle_t hQueue = NULL;

bool checkParity(const uint8_t von, const uint8_t bis) {
  static uint8_t sum;
  static uint8_t i;

  sum = 0;
  for(i=von; i <= bis; ++i) 
    sum += frame[i];
  return sum % 2 == 0;
}

void analysis(void* param) {
    bool p1 = false;
    bool p2 = false;
    bool p3 = false;
    bool valid = false;

    for(;;) {
      if(minStart) {
          dtDCF.tm_hour = frame[29] + (frame[30] << 1) + (frame[31] << 2) + (frame[32] << 3) + frame[33] * 10 + frame[34] * 20;
          dtDCF.tm_min  = frame[21] + (frame[22] << 1) + (frame[23] << 2) + (frame[24] << 3) + frame[25] * 10 + frame[26] * 20 + frame[27] * 40;
          dtDCF.tm_mday = frame[36] + (frame[37] << 1) + (frame[38] << 2) + (frame[39] << 3) + frame[40] * 10 + frame[41] * 20;
          dtDCF.tm_wday = frame[42] + (frame[43] << 1) + (frame[44] << 2);
          dtDCF.tm_mon  = frame[45] + (frame[46] << 1) + (frame[47] << 2) + (frame[48] << 3) + frame[49] * 10;
          dtDCF.tm_year = frame[50] + (frame[51] << 1)  + (frame[52] << 2) + (frame[53] << 3) + frame[54] * 10 + frame[55] * 20 + frame[56] * 40 + frame[57] * 80;
#ifdef ESP32_DEBUG       
          Serial.print(&dtDCF, "%A, %B %d %Y %H:%M:%S");
          Serial.printf("%d:%2d - %d.%d.%d wday:%d\n",dtDCF.tm_hour,dtDCF.tm_min,dtDCF.tm_mday,dtDCF.tm_mon,dtDCF.tm_year,dtDCF.tm_wday);
          Serial.printf("BIT: %d\n",bit);
          Serial.printf("Prüfbits P20:%d Pb1:%d Pb2:%d Pb3:%d\n",frame[20],frame[28],frame[35],frame[58]);
#endif
          validDCF = false;
          valid = bit == 59 && frame[20];   // Startbit = 1
          if(valid) {
            p1 = checkParity(21, 28); 
            p2 = checkParity(29, 35); 
            p3 = checkParity(36, 58); 
            validDCF = p1 && p2 && p3;
#ifdef ESP32_DEBUG
            Serial.printf("Pmin:%d Phour:%d Pdate:%d validDCF:%d\n",p1,p2,p3,validDCF);
#endif
            }
            if(validDCF) {
              xQueueSend(hQueue, &dtDCF, (TickType_t) 0);
            }
#ifdef ESP32_DEBUG            
            Serial.printf("Bit:%d P20:%d|Pm:%d Ph:%d Pd:%d",bit,frame[20],p1,p2,p3);
 #endif
            bit = 0;
            minStart = false;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void IRAM_ATTR isrP15() {
  static unsigned long loStart = 0;
  static unsigned long hiStart;
  static unsigned long diffTime;
  static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  if(digitalRead(DCF_PIN)) {
     hiStart = millis();
     diffTime= hiStart - loStart;
     if(diffTime > 860 && diffTime < 960) {
       if(bit < 59) {
        frame[bit] = 0;
       }
     }
     else if(diffTime > 760 && diffTime < 860) {
       if(bit < 59) {
        frame[bit] = 1;
       }
     }
     else if(diffTime < 500) {
      bit = 0;
     }
     portENTER_CRITICAL_ISR(&mux);
     fallingEdge = false;
     portEXIT_CRITICAL_ISR(&mux);
   }
   else {
    loStart = millis();
    if(diffTime > 1700UL) {
       minStart = true;
    }
    ++bit;
    portENTER_CRITICAL_ISR(&mux);
    fallingEdge = true;
    portEXIT_CRITICAL_ISR(&mux);
  }
}

void dcfInit() {
    pinMode(DCF_PIN, INPUT_PULLUP);
    hQueue = xQueueCreate(1, sizeof(struct tm));
    attachInterrupt(DCF_PIN, isrP15, CHANGE);
    xTaskCreate(analysis,"analysis",8192,NULL,1,NULL);  
}

void setupDCF() {
    dcfInit();
    t1 = millis();
}

void evalDCF() {
    static unsigned long t2;
    static bool status = false;
    static struct tm dtDCF;
    if(fallingEdge && status != fallingEdge) {
     // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));         // toggle LED
    }
    status = fallingEdge;
    t2 = millis();
    if((t2 - t1)  > 995) {
      analysis(NULL);
      t1 = t2;
    }
    delay(20);
}