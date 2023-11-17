#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <cJSON.h>
#include <Wire.h>
#include <BLEScan.h>
#include <BLEDevice.h>
#include <BLEAdvertisedDevice.h>
#include "U8g2lib.h"
#include "DHTesp.h"
// #define _DEBUG_ no Serial.print
#define DHT_PIN 10  // Pin 18 ESP32
#define MOTION_PIN 9
#define CLOUDY 64
#define PARTLY_CLOUDY 65
#define INDOOR 64
#define OUTDOOR 65
#define NIGHT 66
#define RAINY 67
#define STARRY 68
#define SUNNY 69
#define PERCENT 37
#define ESP32_DEBUG   // for dubug purpose

const char*  ssid    = "Vodafone-CF6C";
const char* password = "7HGZ2eGXrTFpbGLE";
const char* addr = "http://api.openweathermap.org/data/2.5/weather?q=Dresden,DE&lang=de&APPID=41be67d28f623524876fb85fdc8f5cb3";
const String sPrefix("4c0002154d6fc88bbe756698da486866a36ec78e");
const int scanTime = 5; //In seconds

struct weather_data {
  int humidity;
  int pressure; 
  int temperature; 
  int clouds;
  uint32_t sunset;
  uint32_t secs;
  String weather;
  String description;
  struct tm now;
};

struct minmax_data {
    int temperature;
    struct tm now;
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

const unsigned char bitmap_icon_battery [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x1F, 0x02, 0x20, 
  0xDA, 0x66, 0xDA, 0x66, 0xDA, 0x66, 0x02, 0x20, 0xFC, 0x1F, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

static char buffer[36];
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
DHTesp dht;
hw_timer_t *tim0 = nullptr;
uint8_t displayEnable;
uint16_t percent = 0;
int outSideTemp = 0;
int inSideTemp = 0;
int inSideHumidity = 0;
BLEScan *pBLEScan;
uint32_t callCounter = 0;
unsigned long t1;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
xQueueHandle hQueueRequestTask;
TaskHandle_t tempTaskHandle = NULL;
TaskHandle_t reqTaskHandle  = NULL;
volatile SemaphoreHandle_t timerSemaphore;

bool validDate = false;
bool validForecast = false;
bool validInTemp = false;
bool validOutTemp = false;
bool doRequest = true;
bool doScroll = false;

typedef void (*func_type)(uint8_t&, uint8_t&, tm& dt, weather_data& wd);

std::string millisToTimeString(const uint32_t& millis) {
    char buffer[10] = "\0";
    uint32_t s = millis / 1000;
    uint32_t m = 0;
    uint32_t h = 0;
    
    m = s / 60;
    s %= 60; 
    h = m / 60;
    m %= 60;
    h %= 24;
    sprintf(buffer, "%02d:%02d:%02d",h,m,s);
    return std::string(buffer);
}

void checkMinMax(minmax_data& xmin, minmax_data& xmax, const uint16_t& c, tm& dt) {
    if(c > xmax.temperature) {
        xmax.temperature = c;
        xmax.now = dt;
    }
    if(c < xmin.temperature) {
        xmin.temperature = c;
        xmin.now = dt;
    }
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
                if(callCounter != count) {
                    uint16_t volt = p[0] + (p[1] << 8);
                    uint16_t temp = p[2] + (p[3] << 8);
                    uint32_t time = p[8] + (p[9] << 8) + (p[10] << 16) + (p[11] << 24);
                    callCounter = count;
                    for(uint8_t i= 0; i < 6; i++) {
                        if(volt >  tab[i].mv) {     // get charge level
                            percent = tab[i].pc;    // from the table
                            break;
                        }
                    }
                    portENTER_CRITICAL(&mux);
                        outSideTemp = (int)temp;
                        validOutTemp = true;
                        t1 = millis();
                    portEXIT_CRITICAL(&mux);
#ifdef _DBUG_                    
                    Serial.printf("U:%d T:%d Count:%d Time:%s\n",volt,outSideTemp,count,millisToTimeString(time).c_str());
#endif
                }
            }
        }
    }
};

void IRAM_ATTR isrTimer() {
    xSemaphoreGiveFromISR(timerSemaphore,NULL); 
}

void IRAM_ATTR isrMotion() {
  displayEnable = digitalRead(MOTION_PIN);
}

bool isLeapyear(const uint16_t& year) {
	  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint32_t epochTime(const tm& dt) {
    uint16_t startYear = 1970UL;
    uint16_t days = 0;

    for (uint16_t& year = startYear; year < dt.tm_year; ++year) {
      days += 365;
      if(isLeapyear(year))
        ++days;
    }
    days += dt.tm_yday;
    uint32_t sec = days * 24 * 60 * 60;
    sec += dt.tm_sec + dt.tm_min * 60 + dt.tm_hour * 60 * 60;
    return sec;
}


// Reads the actual time from an internet time server
// Get current weather forecast - hourly  
void requestTask(void* param) {
    static struct weather_data wd;
    struct tm dtWeb;
    HTTPClient http;

    for(;;) {
        if(WiFi.status() == WL_CONNECTED && doRequest) {
            configTime(3600,3600,"de.pool.ntp.org");
            if(getLocalTime(&dtWeb)) {
               portENTER_CRITICAL(&mux);
                   ++dtWeb.tm_mon;
                   wd.now = dtWeb;
                   ++wd.now.tm_wday;
                   wd.now.tm_year %= 100;
                   wd.secs = epochTime(dtWeb);
                   validDate = true;
                portEXIT_CRITICAL(&mux);
            }

            if(http.begin(addr)) {
                validForecast = http.GET() == HTTP_CODE_OK;
                if(validForecast) {
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
                        wd.weather = cJSON_GetObjectItem(weather, "main")->valuestring;
                        wd.description = cJSON_GetObjectItem(weather, "description")->valuestring;
                        cJSON* clouds =  cJSON_GetObjectItem(root, "clouds");
                        wd.clouds = cJSON_GetObjectItem(clouds, "all")->valueint;
                        cJSON* sys = cJSON_GetObjectItem(root, "sys");
                        wd.sunset = (uint32_t)cJSON_GetObjectItem(sys, "sunset")->valuedouble;
                    portEXIT_CRITICAL(&mux);
                    cJSON_Delete(root);
                }
            }

            doRequest = validDate && validForecast;
            if(doRequest) {
                xQueueSend(hQueueRequestTask,&wd, 10 / portTICK_PERIOD_MS);
#ifdef _DEBUG_
                Serial.print("Vorhersage ");   
                sprintf(buffer, "aktualisiert um %02d:%02d",wd.now.tm_hour,wd.now.tm_min);
                Serial.println(buffer);
                sprintf(buffer, "%02d°C %03d W:%d",wd.temperature,wd.humidity,wd.clouds);
                Serial.println(buffer);
                Serial.print("Weather:");
                Serial.println(wd.weather);
                Serial.print("Description:");
                Serial.println(wd.description);
#endif
                doRequest = false;
            }      
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void bleBeaconTask(void* param) {
    static uint32_t t2 = millis();
    
    for(;;) {
      uint32_t t3 = millis();
      if(t3 - t2 > 10000) {
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
        t2 = t3;
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void insideTemperatureTask(void* param) {
    while(1) {
        TempAndHumidity tah = dht.getTempAndHumidity();
        if(dht.getStatus() == 0) {
            inSideTemp = tah.temperature;
            inSideHumidity   = tah.humidity;
            validInTemp = true;
#ifdef _DEBUG_
            Serial.printf("Tin:%d Humidity:%d\n",inSideTemp,inSideHumidity);
#endif
            vTaskSuspend(NULL);
        }
        else {
           vTaskDelay(10 / portTICK_PERIOD_MS); 
        }
    }
}

//  Shows continuously the weather description by scrolling from right to left
void drawScrollString(struct weather_data& wd) {
    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_8x13_mf);
    const static uint8_t SW = u8g2.getUTF8Width(wd.description.c_str());
    const static uint8_t OFS = SW + 96 >= u8g2.getDisplayWidth() ?
      u8g2.getDisplayWidth()-1 : SW + 96;
    static uint8_t offset = OFS;
    
    u8g2.drawUTF8(16 - SW + offset,33, wd.description.c_str());     // draw the scolling text
    u8g2.updateDisplayArea(2,3,12,2);
    offset--;								                        // scroll by one pixel
    if(offset == 0)	{
        offset = OFS;			                                    // start over again
    }
}

void printTimeLine(uint8_t x, uint8_t y, const uint8_t* bigFont,const uint8_t* smallFont, tm& tmTime, bool showSec=true) {
    sprintf(buffer,"%d:%02d ",tmTime.tm_hour,tmTime.tm_min);
    u8g2.setFontPosBottom();
    y = u8g2.getDisplayHeight();
    u8g2.setFont(bigFont);
    u8g2.drawStr(x,y, buffer);
    if(showSec) {
        x += u8g2.getStrWidth(buffer) - u8g2.getMaxCharWidth()/2;
        sprintf(buffer,"%02d",tmTime.tm_sec);
        u8g2.setFont(smallFont);
        u8g2.drawStr(x,y, buffer); 
    }
}

void printTimeDate(uint8_t& x, uint8_t& y, tm& dt, weather_data& wd) {
    static const char* DAYS[] = {"","So","Mo","Di","Mi","Do","Fr","Sa"};
    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_logisoso16_tf);
    x = 0;

    if(validDate) {        // Datum wurde aktualisiert
        if(dt.tm_wday > 0 && dt.tm_wday <= 7)      
            sprintf(buffer,"%s, %d.%d.%02d",DAYS[dt.tm_wday],dt.tm_mday,dt.tm_mon,dt.tm_year);
        else
            sprintf(buffer,"%d.%d.%02d",dt.tm_mday,dt.tm_mon,dt.tm_year);
    
        u8g2.drawStr(x,0, buffer); 
    }
    sprintf(buffer,"%d:%02d:%02d",dt.tm_hour,dt.tm_min,dt.tm_sec);
    x = 24; 
    printTimeLine(x,y,u8g2_font_logisoso24_tf,u8g2_font_7x14_tf,dt);
}

void printWeather(uint8_t& x, uint8_t& y, tm& dt, weather_data& wd) {
    static uint8_t pos;
   
    x = 0;
    y = 0;
    if(validForecast) {
        doScroll = true;
        u8g2.setFontPosTop();
        sprintf(buffer, "%d",wd.temperature);
        u8g2.setFont(u8g2_font_logisoso16_tf);
        u8g2.drawStr(x,y, buffer);
        x = u8g2.getStrWidth(buffer) + 3;
        u8g2.setFont(u8g2_font_t0_12_mf);
        u8g2.drawGlyph(x,y, 176);
        x += 6;
        u8g2.drawStr(x,0, "C");
    
        u8g2.setFont(u8g2_font_t0_12_mf);
        sprintf(buffer,"%s", "mBar");
        x = u8g2.getDisplayWidth() - u8g2.getStrWidth(buffer);
        u8g2.drawStr(x,4, buffer);
        sprintf(buffer,"%d", wd.pressure);
        u8g2.setFont(u8g2_font_logisoso16_tf);
        x -= (u8g2.getStrWidth(buffer)+4);
        u8g2.drawStr(x,y,buffer);
        
        u8g2.setFontPosBottom();
        if(wd.weather.equals("Clear")) {
            if(wd.secs> wd.sunset)
                pos = STARRY;
            else
                pos = SUNNY;
        }
        else if(wd.weather.equals("Rain"))
            pos = RAINY;
        else if(wd.weather.equals("Clouds")) {
            if(wd.clouds < 50)
                pos = PARTLY_CLOUDY;
            else
                pos = CLOUDY;
        }
    
        u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
        u8g2.drawGlyph(0,u8g2.getDisplayHeight(), pos);
        u8g2.setFont(u8g2_font_t0_16_mf);
        x = u8g2.getDisplayWidth() - u8g2.getStrWidth("xx:xx:xx");
        printTimeLine(x,y,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf,dt);
    }
    else {
        printTimeDate(x, y, dt, wd);
    }
}

void printMinMaxTemperatures(minmax_data& minTemp, minmax_data& maxTemp, uint16_t icon) {
    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_t0_14_mf);
    u8g2.drawStr(25,0, "Min");
    u8g2.drawStr(88,0, "Max");
    u8g2.setFont(u8g2_font_t0_16_mf);
    sprintf(buffer,"%2d",minTemp.temperature);
    u8g2.drawStr(25,18, buffer);
    sprintf(buffer,"%2d",maxTemp.temperature);
    u8g2.drawStr(78,18, buffer);
    
    printTimeLine(0,48,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf,minTemp.now),false;
    printTimeLine(65,48,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf,maxTemp.now,false);
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(0,40,icon);
    u8g2.setFont(u8g2_font_t0_14_mf);
    u8g2.drawGlyph(104,28, 176);
    u8g2.drawStr(112,28, "C");
}

void printHumidities(tm& dt, weather_data& wd) {
    int x = 0;
    
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x,0, INDOOR);
    u8g2.setFont(u8g2_font_logisoso16_tf);
    sprintf(buffer,"%d", inSideHumidity);        
    x = 80;
    u8g2.drawStr(x,2, buffer);            // Raumfeuchte
    x += u8g2.getStrWidth(buffer) + 2;
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawGlyph(x,2,PERCENT);      // draw %

    x = 0;
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.drawGlyph(x,25, OUTDOOR);
    if(validForecast) {
        u8g2.setFont(u8g2_font_logisoso16_tf);
        sprintf(buffer,"%d", wd.humidity);        
        x = 80;
        u8g2.drawStr(x,27, buffer);
        x += u8g2.getStrWidth(buffer) + 2;
        u8g2.setFont(u8g2_font_unifont_t_symbols);
        u8g2.drawGlyph(x,27,PERCENT);      
    }
    
    u8g2.setFont(u8g2_font_t0_16_mf);
    x = u8g2.getDisplayWidth() + 5 - u8g2.getStrWidth("xx:xx:xx");
    printTimeLine(x,50,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf,dt);   
}

void printTemperatures(uint8_t& x, uint8_t& y, tm& dt, weather_data& wd) {
    static minmax_data outMinTemp = {100};
    static minmax_data outMaxTemp = {-100};
    static minmax_data inMinTemp  = {100};
    static minmax_data inMaxTemp  = {-100};
    
    if(millis() - t1 > 300000UL) {  // 5min without BLE-signal
        validOutTemp = false;        
    }
    u8g2.setFontPosTop();
    x = 0;
    y = 0;
    doScroll = false;

    if(dt.tm_sec == 40) {
        if(validInTemp) {
            checkMinMax(inMinTemp,inMaxTemp,inSideTemp,dt);
        }
        if(validOutTemp) {
            checkMinMax(outMinTemp,outMaxTemp,outSideTemp,dt); 
        }
    }

    if(validInTemp && dt.tm_sec >= 48 && dt.tm_sec <= 51 && abs(inMinTemp.temperature) != abs(inMaxTemp.temperature)) {
        printMinMaxTemperatures(inMinTemp,inMaxTemp, INDOOR);
        return;
    }

    if(validOutTemp) {
        if(dt.tm_sec > 51 && dt.tm_sec <= 55 && abs(outMinTemp.temperature) != abs(outMaxTemp.temperature)) {
            printMinMaxTemperatures(outMinTemp,outMaxTemp, OUTDOOR);
            return;
        }
        else if(dt.tm_sec > 55 && dt.tm_sec <= 59) {
            printHumidities(dt, wd);
            return;
        }
    }
    else if(dt.tm_sec > 51 && dt.tm_sec <= 59) {
            printHumidities(dt, wd);
            return;
    }
    
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    const uint8_t DIS = u8g2.getMaxCharWidth() + u8g2.getMaxCharWidth()/2;
    u8g2.drawGlyph(x,y, INDOOR);

    if(validInTemp) {
        x += DIS;
        u8g2.setFont(u8g2_font_logisoso16_tf);
        sprintf(buffer,"%2d", inSideTemp);         // int-Anteil
        u8g2.drawStr(x,y, buffer);                 // Innentemperatur
    }
    
    x = 0;
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    y += DIS;
    u8g2.drawGlyph(x,y, OUTDOOR);
    x = u8g2.getDisplayWidth() - DIS;
    y -= DIS/2;
    u8g2.setFont(u8g2_font_t0_14_mf);
    u8g2.drawGlyph(x,y, 176);
    x += u8g2.getMaxCharWidth();
    u8g2.drawStr(x,y, "C");

    if(validOutTemp)  {
        x = DIS;
        y += DIS/2;
       
        u8g2.setFont(u8g2_font_logisoso16_tf);
        sprintf(buffer,"%2d",outSideTemp);  
        u8g2.drawStr(x,y, buffer);      // Aussentemperatur
        sprintf(buffer, "%d", percent);
        u8g2.setFont(u8g2_font_t0_12_mf);
        u8g2.drawStr(20,52,buffer);
        int z = 20 + u8g2.getStrWidth(buffer);
        u8g2.setFont(u8g2_font_unifont_t_symbols);
        u8g2.drawGlyph(z,52,PERCENT);      // draw %
    }
    
    u8g2.drawXBMP(0,48,16, 16, bitmap_icon_battery);
    u8g2.setFont(u8g2_font_t0_16_mf);
    x = u8g2.getDisplayWidth() + 5 - u8g2.getStrWidth("xx:xx:xx");
    printTimeLine(x,y,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf,dt);   
}

void initTimer() {
    tim0 = timerBegin(0, 80, true);
    timerAttachInterrupt(tim0, &isrTimer, true);
    timerAlarmWrite(tim0, 1000000, true); // every sec
    timerAlarmEnable(tim0);
}

void initBLE() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99); // less or equal setInterval value
    xTaskCreate(bleBeaconTask,"bleBeaconTask",8192,NULL,tskIDLE_PRIORITY,NULL); 
}

void setup() {
    pinMode(DHT_PIN,INPUT_PULLUP);
    pinMode(MOTION_PIN, INPUT_PULLUP);
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.setFlipMode(1);    // 180°
    WiFi.begin(ssid, password);
    dht.setup(DHT_PIN, DHTesp::AM2302);   // alternate sensor
#ifdef _DEBUG_    
    Serial.begin(9600);
    while(!Serial);
#endif

    initTimer();
    initBLE();
    attachInterrupt(digitalPinToInterrupt(D9), isrMotion, CHANGE);
    timerSemaphore = xSemaphoreCreateBinary();
    hQueueRequestTask = xQueueCreate(1, sizeof(weather_data));
    xTaskCreate(requestTask,"request",8192,NULL,tskIDLE_PRIORITY,&reqTaskHandle); 
    xTaskCreate(insideTemperatureTask,"insideTemperatureTask",4000,NULL,tskIDLE_PRIORITY,&tempTaskHandle);
    t1 = millis();
}

void loop() {
    static func_type functions[3] = {&printTimeDate,&printWeather,&printTemperatures};
    static struct tm dt;
    static struct weather_data wd;
    uint8_t x;
    uint8_t y;

    if(xQueueReceive(hQueueRequestTask,&wd, 10 / portTICK_PERIOD_MS) == pdTRUE) {
#ifdef _DEBUG
        Serial.printf("Request-Queue-Time:%d:%d\n",wd.now.tm_hour,wd.now.tm_min);
#endif
        dt = wd.now;
    }
    
    if(xSemaphoreTake(timerSemaphore, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        if(++dt.tm_sec > 59) {
            dt.tm_sec = 0;
            if(tempTaskHandle) {
                xTaskResumeFromISR(tempTaskHandle);
            }
            if(++dt.tm_min > 59) {
                dt.tm_min = 0;
                if(reqTaskHandle) {
                    xTaskResumeFromISR(reqTaskHandle);
                }
                doRequest = true;       // every hour 
                if(++dt.tm_hour > 23) {
                    dt.tm_hour = 0;
                }
            }
        }

 //       u8g2.setPowerSave(!displayEnable);
 //       if(displayEnable) {
            u8g2.clearBuffer();
            functions[dt.tm_sec/20](x,y,dt,wd); 
            u8g2.sendBuffer(); 
 //       }
    }

    if(doScroll) {
        drawScrollString(wd);              // Ausgabe Laufschrift
    }
    else {
        delay(100);
    }
}
