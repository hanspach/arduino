#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <cJSON.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BLEScan.h>
#include <BLEDevice.h>
#include <BLEAdvertisedDevice.h>
#include "privatedata.h"
//#define _DEBUG_

 struct weather_data {
  int humidity;
  int pressure; 
  int temperature; 
  int clouds;
  String sunrise;
  String sunset;
  String iconurl;
  String weather;
  String description;
};

// Entladetabelle Battery
// Spannung(V)   Ladezustand(%)
//    4,2           100
//    4,1            90
//    4,0            80
//    3,9            60
//    3,8            40
//    3,7            20
//    3,6             0
struct Tab {
  uint16_t mv;
  uint8_t pc;
};
Tab tab[6] = {{4180,100},{4080,90},{3980,80},{3880,60},{3780,40},{3680,20}};

AsyncWebServer server(80);
OneWire oneWire(15);
DallasTemperature ds1820(&oneWire);
struct tm dt = {0};
struct weather_data wd = {0};
int inSideTemp = 0;

uint16_t percent = 0;
int outSideTemp = 0;
BLEScan *pBLEScan;
uint32_t callCounter = 0;
hw_timer_t *tim0 = nullptr;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
uint8_t minutes = 0;
volatile bool doRequest = true;
volatile bool doMeasure = true;
volatile bool doScan = false;
bool validTemp  = false;
bool validBleData = false;
bool validRequest = false;

void IRAM_ATTR isrTimer() {
  portENTER_CRITICAL(&mux);
  if(dt.tm_sec % 10 == 0) {
    doScan = true;
  }
  if(++dt.tm_sec > 59) {
      dt.tm_sec = 0;
      if(dt.tm_min % 2 == 1) {
        doMeasure = true;
      }
      if(minutes > 5) { // no BLE-signal
        validBleData = false;
      }
      else {
        ++minutes;
      }
      if(++dt.tm_min > 59) {
          dt.tm_min = 0;
          doRequest = true;
          if(++dt.tm_hour > 23) {
              dt.tm_hour = 0;
          }
      }
  }
  portEXIT_CRITICAL(&mux);
}

// Reads the actual time from an internet time server
// Get current weather forecast - hourly  
void requestTask(void* param) {
  struct tm dtWeb;
  HTTPClient http;
  
  for(;;) {
    if(doRequest) {
      doScan = false; 
      bool validDate = false;
      bool validWeatherData = false;
      
      configTime(3600,3600,"de.pool.ntp.org");
      if(getLocalTime(&dtWeb)) {
        portENTER_CRITICAL(&mux);
        ++dtWeb.tm_mon;
        dt = dtWeb;
        ++dt.tm_wday;
        dt.tm_year %= 100;
        portEXIT_CRITICAL(&mux);
        validDate = true;
      }

      if(http.begin(ADDR)) {
        if(http.GET() == HTTP_CODE_OK) {
          String js = http.getString();
          cJSON* root = cJSON_Parse(js.c_str());
          cJSON* main = cJSON_GetObjectItem(root, "main"); 
          portENTER_CRITICAL(&mux);
          wd.humidity = cJSON_GetObjectItem(main, "humidity")->valueint;
          float t = cJSON_GetObjectItem(main, "temp")->valuedouble;
          wd.temperature = (int)(t - 273.15);
          wd.pressure = cJSON_GetObjectItem(main, "pressure")->valueint;
          cJSON* ar = cJSON_GetObjectItem(root, "weather");
          cJSON* weather = cJSON_GetArrayItem(ar, 0);
          String icon = cJSON_GetObjectItem(weather, "icon")->valuestring;
          wd.weather = cJSON_GetObjectItem(weather, "main")->valuestring;
          wd.description = cJSON_GetObjectItem(weather, "description")->valuestring;
          cJSON* clouds =  cJSON_GetObjectItem(root, "clouds");
          wd.clouds = cJSON_GetObjectItem(clouds, "all")->valueint;
 
          if(!icon.isEmpty()) { 
            wd.iconurl = "https://openweathermap.org/img/wn/";
            wd.iconurl += icon;
            wd.iconurl += "@2x.png";
          }

          portEXIT_CRITICAL(&mux);
          cJSON_Delete(root);
          validWeatherData = true;
        }
      }
#ifdef _DEBUG_
      Serial.printf("Vorhersage um %d:%02d %s\n",dt.tm_hour,dt.tm_min,wd.description.c_str());
#endif
      http.end();
      validRequest = validDate && validWeatherData;
      doRequest = false;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);   
  }
}

void temperatureTask(void* param) {
    while(1) {
      if(doMeasure) {
        ds1820.requestTemperatures(); 
        float c = ds1820.getTempCByIndex(0);
        if(!isnan(c)) {
            portENTER_CRITICAL(&mux);
                inSideTemp = (int)c;
                validTemp = true;
            portEXIT_CRITICAL(&mux);  
        }  
#ifdef _DEBUG_
        Serial.printf("Tin:%d\n",inSideTemp);
#endif
        doMeasure = false;
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void response(AsyncWebServerRequest* req) {
  static char buffer[15] = {0};
  static const char* DAYS[] = {"","So","Mo","Di","Mi","Do","Fr","Sa"};
  String content = "{ ";
  doScan = false;
  pBLEScan->stop();

  if(validRequest) {
    sprintf(buffer,"\"%s, %d.%d.%02d\"",DAYS[dt.tm_wday],dt.tm_mday,dt.tm_mon,dt.tm_year);
    content += "\"date\": ";
    content += buffer;
 
    content += ", \"hour\": ";
    content += String(dt.tm_hour);
    content += ", \"min\": ";
    content += String(dt.tm_min);
    content += ", \"sec\": ";
    content += String(dt.tm_sec);
    content += ", \"humidity\": ";
    content += String(wd.humidity); 
    content += ", \"temperature\": ";
    content += String(wd.temperature);
    content += ", \"pressure\": ";
    content += String(wd.pressure);
    content += ", \"clouds\": ";
    content += String(wd.clouds);
    
    content += ", \"description\": ";
    sprintf(buffer, "\"%s\"", wd.description.c_str());
    content += buffer;
    if(!wd.iconurl.isEmpty()) {
      content += ", \"iconurl\": \"";
      content +=  wd.iconurl;
      content += "\"";
    }
    content += ", ";
  }
  if(validTemp) {
    content += "\"itemp\": ";
    content += String(inSideTemp);
  }
  if(validBleData) {
    content += ", \"otemp\": ";
    content += String(outSideTemp);
    content += ", \"batpercent\": ";
    content += String(percent);
  }
  content += " }";
#ifdef _DEBUG_
  Serial.println(content.c_str());
#endif
  req->send(200, "application/json", content);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        uint8_t *payLoad = advertisedDevice.getPayload();                      // search for Eddystone Service Data in the advertising payload
        const size_t payLoadLen = advertisedDevice.getPayloadLength();  // *payload shall point to eddystone data or to its end when not found
        uint8_t *payLoadEnd = payLoad + payLoadLen - 1;                                                 // address of the end of payLoad space
        
        while(payLoad < payLoadEnd) {
            if (payLoad[1] == 0x16 && payLoad[2] == 0xAA && payLoad[3] == 0xFE) {
                payLoad += 4;                                                                               // found!
                break;
            }
            payLoad += *payLoad + 1;                                                                      // payLoad[0] has the field Length
        }

        if (payLoad < payLoadEnd) {                                                                     // EddystoneTLM Service Data were found
            if (*payLoad == 0x20) {
                uint8_t* p = payLoad + 2;
                uint32_t count = p[4] + (p[5] << 8) + (p[6] << 16) + (p[7] << 24);
                uint32_t time  = p[8] + (p[9] << 8) + (p[10]<< 16) + (p[11] << 24);                     // not yet evalueted
                if(callCounter != count) {
                    portENTER_CRITICAL(&mux);
                    uint16_t u = p[0] + (p[1] << 8);
                    int t  = p[2] + (p[3] << 8);
                    if((t & 0x8000) == 0x8000) {
                        t -= 65536;
                    }
                    outSideTemp = t;
                    callCounter = count;
                    minutes = 0;
                    for(uint8_t i= 0; i < 6; i++) {
                        if(u >  tab[i].mv) {        // get charge level
                            percent = tab[i].pc;    // from the table
                            break;
                        }
                    }
                    validBleData = true; 
                    portEXIT_CRITICAL(&mux);
#ifdef _DEBUG_                    
                    Serial.printf("U:%dmV Procent:%d Temp:%d°C\n",u,percent,outSideTemp);
#endif      
                }
            }
        }
    }
};

void setup() { 
#ifdef _DEBUG_  
  Serial.begin(9600);
  while(!Serial);
#endif
  WiFi.begin(SSID, PWD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
  } 
#ifdef _DEBUG_ 
  Serial.println(WiFi.localIP());
#endif 
  ds1820.begin();
  xTaskCreate(requestTask,"request",8192,NULL,tskIDLE_PRIORITY,NULL);
  xTaskCreate(temperatureTask,"temperature",4096,NULL,tskIDLE_PRIORITY,NULL);

  tim0 = timerBegin(0, 80, true);
  timerAttachInterrupt(tim0, &isrTimer, true);
  timerAlarmWrite(tim0, 1000000, true); // every sec
  timerAlarmEnable(tim0);

  server.on("/", HTTP_GET, response);
  server.begin();

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop() {
  if(doScan) {
    BLEScanResults foundDevices = pBLEScan->start(5, false);
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
    doScan = false;
  }
  delay(10);
}



