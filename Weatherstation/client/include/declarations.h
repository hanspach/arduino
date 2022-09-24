#ifndef _DECLARATIONS_H
#define _DECLARATIONS_H
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <DHTesp.h>
#include <cJSON.h>
#include <U8g2lib.h>

#define LED 2
#define DCF_PIN 15
#define DHT_PIN 18
#define CLOUDY 64
#define PARTLY_CLOUDY 65
#define INDOOR 64
#define OUTDOOR 65
#define NIGHT 66
#define RAINY 67
#define STARRY 68
#define SUNNY 69
//#define ESP32_DEBUG   // for dubug purpose

struct weather_data {
  int humidity;
  int pressure; 
  int temperature; 
  int clouds;
  uint32_t sunset;
  uint32_t now;
  String weather;
  String description;
};

void dcfInit();
#endif