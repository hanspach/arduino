#include <Arduino.h>
#include <Adafruit_INA219.h>

const int PWM_FREQ = 5000;
const int PWM_CHAN = 0;
const int PWM_RESO = 10;
const int MAX_CYCL = (int)(pow(2, PWM_RESO)-1);
const int ADU_RESO = 4095;
const int LED_PIN  = 17;
const int ADU_PIN  = 14;

Adafruit_INA219 ina;

void initIna() {
    if(!Serial) {
        Serial.begin(9600);
        while (!Serial) {
            delay(1);
        }
    }
    
    if(!ina.begin()) {
        Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
}

void measureIna() {
    float busvoltage = ina.getBusVoltage_V();

    Serial.print("Spannung:");
    Serial.print(busvoltage);
    Serial.println(" Volt");
}

void initPWM() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(ADU_PIN, INPUT);
    ledcSetup(PWM_CHAN,PWM_FREQ,PWM_RESO);
    ledcAttachPin(LED_PIN, PWM_CHAN);
}

void performPWM() {
    uint16_t dutyCycle = analogRead(ADU_PIN);
    Serial.print("dutyCycle:");
    Serial.println(dutyCycle);
    dutyCycle = map(dutyCycle,0,ADU_RESO,0,MAX_CYCL);
    ledcWrite(PWM_CHAN, dutyCycle);
}