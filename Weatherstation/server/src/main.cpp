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
char txtVal[10];
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic;

void setup() {
  
  //Serial.begin(9600);
  memset(txtVal, '\0', sizeof(txtVal));
  // pinMode(LED_BUILTIN, OUTPUT);  // nur für extern angeordneter LED sinnvoll
  // digitalWrite(LED_BUILTIN,HIGH);
 
  esp_bluedroid_init();
  esp_bluedroid_enable();
  esp_sleep_enable_timer_wakeup(1000000ULL * 120);
  tempsensor.begin(0x18);                           // default address
  tempsensor.setResolution(1);                      //
  
  BLEDevice::init("ESP32BLE_SENSOR");               // Create the BLE Device
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
  pAdvertising->start();
  
  tempsensor.wake();  // wake up, ready to read!
  float f = tempsensor.readTempC();
  int c = (int)f;     // wir runden ab
  // Serial.flush();  // Puffer leeren funktioniert nicht!
  // Serial.println("Messung:"); // 1. Ausgabe = Datenmüll
  sprintf(txtVal,"%2d",c);
  
  //Serial.println(txtVal);
  pCharacteristic->setValue(txtVal);
  pCharacteristic->notify();
  delay(2000);
  tempsensor.shutdown_wake(1);
  
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  delay(200);
  esp_deep_sleep_start();
}

void loop() {
}
