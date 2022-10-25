#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_bt_main.h>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
char txtVal[10] = "\0";
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic;
RTC_DATA_ATTR unsigned int count = 1;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  Serial.begin(9600);
  esp_sleep_enable_timer_wakeup(1000000ULL * 60);
  Serial.print("Setup started: ");
  tempsensor.begin(0x18);                           // default address
  tempsensor.setResolution(1);                      //
  
  BLEDevice::init("SENS");               // Create the BLE Device
  while(!BLEDevice::getInitialized()) {
     vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  pServer = BLEDevice::createServer();              // Create the BLE Server
  BLEService* pService = pServer->createService(SERVICE_UUID);  // Create the BLE Service
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,
  BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();                               // Start the service
  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  tempsensor.wake();                                // wake up, ready to read!
  float f = tempsensor.readTempC();
  int c = (int)f;                                  // round value
  sprintf(txtVal,"%2d", c);
  pCharacteristic->setValue(txtVal);
  pCharacteristic->notify();
  vTaskDelay(2000 / portTICK_PERIOD_MS);            // enough time to send data
  tempsensor.shutdown_wake(1);
  Serial.printf("BLE sends %s°C for %d times.\n",txtVal,count);
  ++count;
  BLEDevice::deinit();
  while(BLEDevice::getInitialized()) {              // disable BLE
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  esp_deep_sleep_start();
}

void loop() {
}
