#include <Arduino.h>
#include <HardwareSerial.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#if CONFIG_IDF_TARGET_ESP32
    #include "esp32/rom/uart.h"
#elif CONFIG_IDF_TARGET_ESP32S2
    #include "esp32s2/rom/uart.h"
#endif

#define TXD_PIN 17
#define RXD_PIN 16
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#define BUF_SIZE 1024

HardwareSerial ser(2);
static intr_handle_t handle_console;
uint8_t rxbuf[256];

void IRAM_ATTR uart_isr(void* param) {
  uint16_t rx_fifo_len, status;
  uint16_t i;
  
  status = UART0.int_st.val; // read UART interrupt Status
  rx_fifo_len = UART2.status.rxfifo_cnt; // read number of bytes in UART buffer
   while(rx_fifo_len){
   rxbuf[i++] = UART2.fifo.rw_byte; // read all bytes
   rx_fifo_len--;
 }
 // after reading bytes from buffer clear UART interrupt status
 uart_clear_intr_status(UART_NUM_2, UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);

 for(uint8_t i=0; i < 256 && rxbuf[i] != '\0'; i++) {
  Serial.print(rxbuf[i]);
 }
}

void initUart() {
  uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};

  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2,
    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);   //Install UART driver, and get the queue.
  uart_isr_free(UART_NUM_2);                                      //release the pre registered UART handler/subroutine
  uart_isr_register(UART_NUM_2,uart_isr, NULL, ESP_INTR_FLAG_IRAM, &handle_console);  // register new UART subroutine
  uart_enable_rx_intr(UART_NUM_2);  // enable RX interrupt
}

void setup() {
 ser.begin(9600,SERIAL_8N1,RXD_PIN,TXD_PIN);
 pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if(ser.available()) {
    int res = ser.read();
    if(res % 2)
        digitalWrite(LED_BUILTIN, HIGH);
    else
        digitalWrite(LED_BUILTIN, LOW);
  }
}

