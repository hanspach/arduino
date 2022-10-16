#include <Arduino.h>
#include <rcswitch.h>
#include <U8g2lib.h>

#define ECHO_TEST_TXD  17
#define ECHO_TEST_RXD  16
#define BUF_SIZE (1024)


//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
RCSwitch receiver = RCSwitch();

void setup() {
  delay(500);
 // u8g2.begin();
  Serial.begin(9600);
  receiver.enableReceive(16);
}

void loop() {
  /*
  u8g2.clearBuffer();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_logisoso16_tf);
  u8g2.setFontPosTop();
  u8g2.drawStr(0,0,msg);
  */
 
  if(receiver.available()) {
      Serial.print("Value:");
      Serial.println(receiver.getReceivedValue());
      receiver.resetAvailable();
  }
  delay(2000);
}