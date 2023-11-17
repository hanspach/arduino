#include "BLEEddystoneTLM.h"
#include <bluefruit.h>

uint32_t BLEEddystoneTLM::count = 0;

BLEEddystoneTLM::BLEEddystoneTLM() {
	uuid = 0xFEAA;
	m_data.frameType = 0x20;
	m_data.version = 0;
    m_data.advCount = ++BLEEddystoneTLM::count;
    m_data.tmil = millis();
} // BLEEddystoneTLM

bool BLEEddystoneTLM::start(void) {
  Bluefruit.Advertising.clearData();
  
  struct ATTR_PACKED {
    uint16_t  eddy_uuid;
    uint8_t   frame_type;
    uint8_t   version;
    uint16_t  volt;
    uint16_t  temp;
    uint32_t  count;
    uint32_t  diffTime;
} eddy =
  {
      .eddy_uuid  = UUID16_SVC_EDDYSTONE,
      .frame_type = 0x20,
      .version    = 0,
      .volt       = m_data.volt,
      .temp       = m_data.temp,
      .count      = m_data.advCount,
      .diffTime   = m_data.tmil
  };

  Bluefruit.Advertising.addUuid(UUID16_SVC_EDDYSTONE);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  return Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_SERVICE_DATA, &eddy, sizeof(eddy));
}

void BLEEddystoneTLM::setData(uint8_t* p) {
    p += 2;                                   // first entry
    m_data.volt = p[0] + (p[1] << 8);
    m_data.temp = p[2] + (p[3] << 8);
    m_data.advCount = p[4] + (p[5] << 8) + (p[6] << 16) + (p[7] << 24);
    m_data.tmil = p[8] + (p[9] << 8) + (p[10] << 16) + (p[11] << 24);
}

uint16_t BLEEddystoneTLM::getVolt() {
    return m_data.volt;
 }
    
uint16_t  BLEEddystoneTLM::getTemp() {
    return m_data.temp;
 }
    
uint32_t  BLEEddystoneTLM::getCount() {
    return m_data.advCount;
 }
    
uint32_t  BLEEddystoneTLM::getTime() {
    return m_data.tmil;
 }

String BLEEddystoneTLM::toString() const {
	String res = "U:";
  res += String(m_data.volt);
  res += "mV T:";
  res += String(m_data.temp);
  res += "°C Count:";
  res += String(m_data.advCount);
  res += " Time:";
  res += String(m_data.tmil);
  return res;
} 

void BLEEddystoneTLM::setVolt(uint16_t volt) {
	m_data.volt = volt;
} // setVolt

void BLEEddystoneTLM::setTemp(float temp) {
	m_data.temp = (uint16_t)temp;        
} // setTemp

void BLEEddystoneTLM::setCount(uint32_t advCount) {
	m_data.advCount = advCount;
} // setCount

void BLEEddystoneTLM::setTime(uint32_t tmil) {
	m_data.tmil = tmil;
} // 