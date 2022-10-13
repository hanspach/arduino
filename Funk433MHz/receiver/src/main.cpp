#include <Arduino.h>
#include <U8g2lib.h>
#include <HardwareSerial.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
#define ECHO_TEST_TXD  17
#define ECHO_TEST_RXD  16
#define BUF_SIZE (1024)


U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static void task(void*) {
  const int uart_num = UART_NUM_1;
  uart_config_t uart_config = {
      .baud_rate = 2400,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    
      .rx_flow_ctrl_thresh = 122,
  }; 

  //Configure UART1 parameters
    uart_param_config(uart_num, &uart_config);
    //Set UART1 pins(TX: IO4, RX: I05)
    uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Install UART driver (we don't need an event queue here)
    //In this example we don't even use a buffer for sending data.
    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);

    while(1) {
        //Read data from UART
        int len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        //Write data back to UART
        Serial.println((const char*) data);
    }
}


HardwareSerial rxs(1);
static char buffer[36] = "\0";



void setup() {
  delay(500);
 // u8g2.begin();
  Serial.begin(9600);
  rxs.begin(1024,SERIAL_8N1,16,17);
 // rxs.onReceive(received);
 // xTaskCreate(received, "received",8192,NULL,1,NULL);
}

void loop() {
  /*
  u8g2.clearBuffer();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_logisoso16_tf);
  u8g2.setFontPosTop();
  u8g2.drawStr(0,0,msg);
  */
  //
     Serial.print("Check UART:");
    while(rxs.available()) {
      char c = rxs.read();
      Serial.print(c);
    } 
    Serial.println();
    delay(2000);
  
}