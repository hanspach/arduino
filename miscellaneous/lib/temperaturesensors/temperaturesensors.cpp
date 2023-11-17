#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MCP9808.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
OneWire oneWire(15);
DallasTemperature ds1820(&oneWire);
static char buffer[36];

void initSensors() {
  u8g2.begin();
  u8g2.enableUTF8Print();
  tempsensor.begin(0x18);                           // default address
  tempsensor.setResolution(1);                      // 
  ds1820.begin();
}

void runSensors() {
    u8g2.clearBuffer();
	u8g2.setFontPosTop();
    float c = tempsensor.readTempC();
    sprintf(buffer,"%2.1f",c);
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(0,0, buffer); 
    int x = u8g2.getStrWidth(buffer) + 10;
    ds1820.requestTemperatures(); 
    c = ds1820.getTempCByIndex(0);
    sprintf(buffer,"%2.1f",c);
    u8g2.drawStr(x,0, buffer); 
	u8g2.sendBuffer();
}