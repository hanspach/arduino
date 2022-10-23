#include <declarations.h>
#include <TimeLib.h>
#include <dcf77.h>

const char*  ssid    = "Vodafone-CF6C";
const char* password = "7HGZ2eGXrTFpbGLE";
const char* addr = "http://api.openweathermap.org/data/2.5/weather?q=Dresden,DE&lang=de&APPID=41be67d28f623524876fb85fdc8f5cb3";
const String sPrefix("4c0002154d6fc88bbe756698da486866a36ec78e");
std::string outsideTemp;
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
static char buffer[36];
struct tm dt;
struct weather_data wd;
DHTesp dht;
unsigned long t1;
unsigned long t3;
unsigned long t4;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice = NULL;
volatile bool validDate = false;
bool validForecast = false;
bool validBleChar = false;
bool doScroll = false;
bool doRequest;
bool WiFiConnected = false;
static bool doConnect = false;
static bool BleConnected = false;

typedef void (*func_type)(uint8_t&, uint8_t&);

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
#ifdef ESP32_DEBUG      
      Serial.println(advertisedDevice.toString().c_str());
#endif
      BLEDevice::getScan()->stop();
      if(myDevice)
        delete myDevice;
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

/**
 * Callback function for the BLE client
 */ 
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    BleConnected = false;
  }
};

/**
 * Measures the time between two BLE-calls
 */ 
#ifdef ESP32_DEBUG
void printDiffTime() {
    unsigned long diff = t4 - t3;
    
    diff /= 1000; // in sec
    Serial.print("Verstrichene Zeit:");
    if(diff > 59) {
      Serial.print(diff/60);
      Serial.print("m:");
    }
    Serial.print(diff%60);
    Serial.println("s");
   
}
#endif

/**
 * "Notify callback for characteristic
 */
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,size_t length, bool isNotify) {
  t4 = millis();
#ifdef ESP32_DEBUG  
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.println((char*)pData);
    printDiffTime();
#endif
    t3 = t4;
    outsideTemp = (char*)pData;
    validBleChar = true;
}

/**
 * Connect to the remote BLE Server.
 * Obtain a reference to the service we are after in the remote BLE server
 * Obtain a reference to the characteristic in the service of the remote BLE server.
 * Read the value of the characteristic.
 */
bool connectToBleServer() {
  BLEClient*  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if(pRemoteService == nullptr) {
#ifdef ESP32_DEBUG
    Serial.print("Failed to find service: ");
    Serial.println(serviceUUID.toString().c_str());
#endif
    pClient->disconnect();
    return false;
  }
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
#ifdef ESP32_DEBUG
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
#endif
    pClient->disconnect();
    return false;
  }
  
  if(pRemoteCharacteristic->canRead()) {
      outsideTemp = pRemoteCharacteristic->readValue();
  #ifdef ESP32_DEBUG
      Serial.print("The characteristic value was: ");
      Serial.println(outsideTemp.c_str());
      printDiffTime();
  #endif
      validBleChar = true;
  }

  if(pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  BleConnected = true;
  return true;
}

bool isLeapyear(const uint16_t& year) {
	return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint32_t epochTime() {
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

/**
 *  Retrieve iterative a Scanner because the server works in deep sleep mode and the BLE connection get lost.
 *  Set the callback we want to use to be informed when we have detected a new device.  
 *  Specify that we want active scanning and start the scan to run for 5 seconds.
 */
void initBLE(void* param) {
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  
  for(;;) {
    pBLEScan->start(5, false);
    if(doConnect) {
      connectToBleServer();
      doConnect = false;
      delay(12000UL);
    }
    delay(2000);
  }
}

/**
 *  Reads the actual time from an internet time server
 *  Get current weather forecast - hourly  and suspends the task
 */ 
void request(void* param) {
  struct tm dtWeb;
  HTTPClient http;
  bool cntd;
  static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  for(;;) {
    cntd = WiFi.status() == WL_CONNECTED;   // bei WiFi-Ausfall
    if(!WiFiConnected && cntd)
      doRequest = true;
    WiFiConnected = cntd;

    if(doRequest) {
      configTime(3600,3600,"de.pool.ntp.org");
      if(getLocalTime(&dtWeb)) {
        portENTER_CRITICAL(&mux);
        validDate = true;
        dt = dtWeb;
        ++dt.tm_mon;
        dt.tm_year %= 100;
        portEXIT_CRITICAL(&mux);
        wd.now = epochTime();
#ifdef ESP32_DEBUG
        Serial.println("validDate");
#endif        
      }
    
      if(http.begin(addr)) {
        validForecast = http.GET() == HTTP_CODE_OK;
        if(validForecast) {
          String js = http.getString();
          cJSON* root = cJSON_Parse(js.c_str());
          cJSON* main = cJSON_GetObjectItem(root, "main"); 
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
          cJSON_Delete(root);
#ifdef ESP32_DEBUG        
          Serial.print("Vorhersage ");
          if(validDate) {
            sprintf(buffer, "aktualisiert um %d:%d",dt.tm_hour,dt.tm_min);
            Serial.println(buffer);
          }
          sprintf(buffer, "%02d°C %03d W:%d",wd.temperature,wd.humidity,wd.clouds);
          Serial.println(buffer);
          Serial.print("Weather:");
          Serial.println(wd.weather);
          Serial.print("Description:");
          Serial.println(wd.description);
#endif
        }
      }
      doRequest = !(validDate && validForecast);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/**
 * Shows continuously the weather description by scrolling from right to left
 */
void drawScrollString() {
  u8g2.setFontPosCenter();
  u8g2.setFont(u8g2_font_8x13_mf);
  const static uint8_t SW = u8g2.getUTF8Width(wd.description.c_str());
  const static uint8_t OFS = SW + 96 >= u8g2.getDisplayWidth() ?
    u8g2.getDisplayWidth()-1 : SW + 96;
  static uint8_t offset = OFS;
  
  u8g2.drawUTF8(16 - SW + offset, 
    33, wd.description.c_str());								  // draw the scolling text
  u8g2.updateDisplayArea(2,3,12,2);
  offset--;								                        // scroll by one pixel
  if(offset == 0)	
    offset = OFS;			                            // start over again
}

void printTimeLine(uint8_t& x, uint8_t& y, const uint8_t* bigFont,const uint8_t* smallFont ) {
  sprintf(buffer,"%d:%02d ",dt.tm_hour,dt.tm_min);
  u8g2.setFontPosBottom();
  y = u8g2.getDisplayHeight();
  u8g2.setFont(bigFont);
  u8g2.drawStr(x,y, buffer);
  x += u8g2.getStrWidth(buffer) - u8g2.getMaxCharWidth()/2;
  sprintf(buffer,"%02d",dt.tm_sec);
  u8g2.setFont(smallFont);
  u8g2.drawStr(x,y, buffer); 
}

void printTimeDate(uint8_t& x, uint8_t& y) {
  static const char* DAYS[] = {"","Mo","Di","Mi","Do","Fr","Sa","So"};
  
  u8g2.setFontPosTop();
  x = 0;
  if(validDate) {
    if(dt.tm_wday > 0 && dt.tm_wday <= 7) 
      sprintf(buffer,"%s, %d.%d.%02d",DAYS[dt.tm_wday],dt.tm_mday,dt.tm_mon,dt.tm_year);
    else
      sprintf(buffer,"%d.%d.%02d",dt.tm_mday,dt.tm_mon,dt.tm_year);
   
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(x,0, buffer); 
  }
  sprintf(buffer,"%d:%02d:%02d",dt.tm_hour,dt.tm_min,dt.tm_sec);
  x = 24; 
  printTimeLine(x,y,u8g2_font_logisoso24_tf,u8g2_font_7x14_tf);
}

void printWeather(uint8_t& x, uint8_t& y) {
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
      if(wd.now > wd.sunset)
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
    printTimeLine(x,y,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf);
  } else {
     printTimeDate(x, y);
  }
}

void printTemperatures(uint8_t& x, uint8_t& y) {
  u8g2.setFontPosTop();
  x = 0;
  y = 0;
  doScroll = false;
  u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
  const uint8_t DIS = u8g2.getMaxCharWidth() + u8g2.getMaxCharWidth()/2;
  u8g2.drawGlyph(x,y, INDOOR);
  
  if(dht.getStatus() == 0) {
    x += DIS;
    u8g2.setFont(u8g2_font_logisoso16_tf);
    static float f = dht.getTemperature();
    static int   c = (int)f;
    sprintf(buffer,"%2d",c);        // int-Anteil
    u8g2.drawStr(x,y, buffer);      // Innentemperatur
  }
  else {
    Serial.println(dht.getStatusString());
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

  if(validBleChar && (millis() - t3) < 150000UL)  {
    x = DIS;
    y += DIS/2;
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(x,y, outsideTemp.c_str());      // Aussentemperatur
  }
  u8g2.setFont(u8g2_font_t0_16_mf);
  x = u8g2.getDisplayWidth() - u8g2.getStrWidth("xx:xx:xx");
  printTimeLine(x,y,u8g2_font_t0_16_mf,u8g2_font_t0_12_mf);   
}

void setup() {
  Serial.begin(9600);
  delay(500);
  
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(ssid, password);
  u8g2.begin();
  u8g2.enableUTF8Print();
  dht.setup(DHT_PIN, DHTesp::AM2302);
  xTaskCreate(request,"Request",8192,NULL,1,NULL);  
  xTaskCreate(initBLE,"initBLE",8192,NULL,1,NULL); 
  DCF77::Start(DCF_PIN);
  doRequest = true;         
  t1 = millis();
  t3 = t1;
}

void loop() {
  static unsigned long t2;
  static func_type functions[3] = {&printTimeDate,&printWeather,&printTemperatures};
  uint8_t x;
  uint8_t y;

  if(doScroll) {
    drawScrollString();
  }
  
  t2 = millis();
  if((t2 - t1)  > 995) {
    if(++dt.tm_sec > 59) {
      dt.tm_sec = 0;
      if(++dt.tm_min > 59) {
        dt.tm_min = 0;
        doRequest = true;       // every hour
        if(++dt.tm_hour > 23) {
          dt.tm_hour = 0;
        } 
      }
    }
    static time_t t = DCF77::getTime();
    if(t != 0) {
      dt = toTmStruct(t);
    }
    u8g2.clearBuffer();
    functions[dt.tm_sec/20](x,y);
    u8g2.sendBuffer(); 
    t1 = t2;
  }
  delay(10);
}
