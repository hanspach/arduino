#include <Arduino.h>
#include <Adafruit_INA219.h>

const int PWM_FREQ = 5000;
const int PWM_CHAN = 0;
const int PWM_RESO = 10;
const int MAX_CYCL = (int)(pow(2, PWM_RESO)-1);
const int ADU_RESO = 4095;
const int OUT_PIN  = 17;
const int ADC_PIN  = 2;

Adafruit_INA219 ina;

void initIna() {
    if(!ina.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1) { delay(10); }
    }
    ina.setCalibration_16V_400mA();
}

void measureIna() {
    float busvoltage =  ina.getBusVoltage_V();
    float current    =  ina.getCurrent_mA();
    float shuntvoltage=  ina.getShuntVoltage_mV();
    Serial.printf("Spannung:%3.1f V Strom:%4.1fmA Shunt:%f3.0mV\n",busvoltage,current,shuntvoltage);
}

void initPWM() {
    pinMode(OUT_PIN, OUTPUT);
    pinMode(ADC_PIN, INPUT);
    ledcSetup(PWM_CHAN,PWM_FREQ,PWM_RESO);
    ledcAttachPin(OUT_PIN, PWM_CHAN);
}

void performPWM() {
    uint16_t dutyCycle = analogRead(ADC_PIN);
    Serial.print("dutyCycle:");
    Serial.println(dutyCycle);
    dutyCycle = map(dutyCycle,0,ADU_RESO,0,MAX_CYCL);
    ledcWrite(PWM_CHAN, dutyCycle);
}