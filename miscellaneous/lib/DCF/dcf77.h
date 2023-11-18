#ifndef _DCF77_H_
#define _DCF77_H_
#include <Arduino.h>

struct DCF77Buffer {
    unsigned long long prefix   :21;
    unsigned long long Min	    :7;	// minutes
    unsigned long long P1		:1;	// parity minutes
    unsigned long long Hour	    :6;	// hours
    unsigned long long P2		:1;	// parity hours
    unsigned long long Day	    :6;	// day
    unsigned long long Weekday  :3; // day of week
    unsigned long long Month    :5; // month
    unsigned long long Year	    :8;	// year (5 -> 2005)
    unsigned long long P3		:1;	// parity
};
    
struct {
    unsigned char parity_flag	:1;
    unsigned char parity_min	:1;
    unsigned char parity_hour	:1;
    unsigned char parity_date	:1;
} flags;

struct error {
    unsigned char loTimeTooShort;
    unsigned char hiTimeTooShort;
    unsigned char bufferPosition;
    unsigned char parityMinute;
    unsigned char parityHour;
    unsigned char parityDate;
};

void dcfInit(int _dcfPin, int _ledPin = -1);
bool dcfDateRequest(struct tm& date);
bool dcfDateRequest(unsigned long& loLong, unsigned long& hiLong);
bool dcfStateRequest(std::string&  data);
#endif 