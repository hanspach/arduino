#include <Arduino.h>
#include <Adafruit_INA219.h>
#include <U8g2lib.h>

Adafruit_INA219 ina;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
    ina.begin();
    u8g2.begin();
}

void loop() {
    static char buffer[5] = "\0";
    double v =  ina.getBusVoltage_V();
    int c =  round(ina.getCurrent_mA());
    
    u8g2.clearBuffer();
	u8g2.setFontPosBottom();
    u8g2.setFont(u8g2_font_t0_22_mf);
    dtostrf(v,3,1,buffer);
    u8g2.drawStr(5,30, buffer); 
    u8g2.setFont(u8g2_font_t0_12_mf);
    u8g2.drawStr(u8g2.getDisplayWidth()-16,30, "V"); 
    u8g2.setFont(u8g2_font_t0_22_mf);
    sprintf(buffer,"%d",c);
    u8g2.drawStr(5,60, buffer); 
    u8g2.setFont(u8g2_font_t0_12_mf);
    u8g2.drawStr(u8g2.getDisplayWidth()-22,60, "mA"); 
    u8g2.sendBuffer();

    delay(50);
}