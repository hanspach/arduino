#include <Arduino.h>
#include <U8g2lib.h>
#include <PinChangeInterruptHandler.h>
#include <RFReceiver.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
RFReceiver receiver(15);
static char buffer[36];

void setup() {
 
  u8g2.begin();
  receiver.begin();
  Serial.println("Init");
}

void loop() {
  u8g2.clearBuffer();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_logisoso16_tf);
  u8g2.setFontPosTop();
  char msg[MAX_PACKAGE_SIZE];
  byte senderId = 0;
  byte packageId = 0;
  byte len = receiver.recvPackage((byte *)msg, &senderId, &packageId);

  Serial.println("");
  Serial.print("Package: ");
  Serial.println(packageId);
  Serial.print("Sender: ");
  Serial.println(senderId);
  Serial.print("Message: ");
  Serial.println(msg);
  u8g2.drawStr(0,0,msg);
 
}