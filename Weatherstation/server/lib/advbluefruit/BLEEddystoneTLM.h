#ifndef _BLEEddystoneTLM_H_
#define _BLEEddystoneTLM_H_
#include <Arduino.h>

class BLEEddystoneTLM {
public:
    BLEEddystoneTLM();
    uint16_t    getVolt();
    int16_t     getTemp();
    uint32_t    getCount();
    uint32_t    getTime();
    bool        start(void);
    void        setData(uint8_t*);    
    void        setVolt(uint16_t volt);
    void        setTemp(float temp);
    void        setCount(uint32_t advCount);
    void        setTime(uint32_t tmil);
    String      toString() const;
private:
    static uint32_t count;
    uint16_t uuid;
    struct ATTR_PACKED {
        uint8_t frameType;
        uint8_t version;
        uint16_t volt;
        int16_t  temp;
        uint32_t advCount;
        uint32_t tmil;
    } m_data;
};
#endif