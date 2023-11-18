#include <Arduino.h>
#include "dcf77.h"


int t1 = 0;
int dcfPin;
int ledPin;
struct error msg;
struct tm dt;
unsigned char bufferPosition = 0;
unsigned char bufferPositionCopy;
unsigned long long dcfBuffer = 0;
unsigned long long dcfBufferCopy;
unsigned char loTimeTooShort = 0;
unsigned char hiTimeTooShort = 0;
bool validDate = false;
volatile bool messageAvailable = false;

void finalizeBuffer() {
    msg = {0,0,0,0,0,0};
	validDate = false;
    msg.loTimeTooShort = loTimeTooShort;
    msg.hiTimeTooShort = hiTimeTooShort;
    if(bufferPosition == 59) {
        struct DCF77Buffer* rx_buffer = (struct DCF77Buffer*)(uint64_t)&dcfBuffer;
        if(flags.parity_min == rx_buffer->P1  &&
            flags.parity_hour == rx_buffer->P2  && flags.parity_date == rx_buffer->P3) {
            dt.tm_sec  = 0;
            dt.tm_min  = rx_buffer->Min-((rx_buffer->Min/16)*6);
            dt.tm_hour = rx_buffer->Hour-((rx_buffer->Hour/16)*6);
            dt.tm_mday = rx_buffer->Day-((rx_buffer->Day/16)*6);
            dt.tm_mon  = rx_buffer->Month-((rx_buffer->Month/16)*6);
            dt.tm_year = 2000 + rx_buffer->Year-((rx_buffer->Year/16)*6);
            validDate = true;
        }
        else {
            if(flags.parity_min  != rx_buffer->P1)  { ++msg.parityMinute; }
            if(flags.parity_hour != rx_buffer->P2)  { ++msg.parityHour; }
            if(flags.parity_date != rx_buffer->P3)  { ++msg.parityDate; }
        }
    }
    else {
        msg.bufferPosition = bufferPosition;
    }

    dcfBufferCopy       = dcfBuffer;
    bufferPositionCopy  = bufferPosition;
    dcfBuffer       = 0;
	bufferPosition  = 0;
    loTimeTooShort  = 0;
    hiTimeTooShort  = 0;
    messageAvailable = true;
}

void appendSignal(unsigned char signal) {
	dcfBuffer |= ((unsigned long long) signal << bufferPosition);
    if(bufferPosition ==  21 || bufferPosition ==  29 || bufferPosition ==  36) {
        flags.parity_flag = 0;
    } 
    // save the parity when the corresponding segment ends
    if (bufferPosition ==  28) {flags.parity_min  = flags.parity_flag;}
    if (bufferPosition ==  35) {flags.parity_hour = flags.parity_flag;}
    if (bufferPosition ==  58) {flags.parity_date = flags.parity_flag;}
    // When we received a 1, toggle the parity flag
    if (signal == 1) {
        flags.parity_flag = flags.parity_flag ^ 1;
    }
	
    bufferPosition++;
	if(bufferPosition > 59) {  // buffer full before min-takt reached
		finalizeBuffer();
	}
}

void IRAM_ATTR dcf_isr() {
    int t2 = millis();
    int diffTime = t2 - t1;

    if(digitalRead(dcfPin)) {
        if(ledPin != -1) {
                digitalWrite(ledPin, !digitalRead(ledPin));
        }
        if(diffTime < 700) {
            ++loTimeTooShort;
        }
        else if(diffTime > 1500) {
            finalizeBuffer();
        }
    }
    else {
        if(diffTime > 50) {
            if(diffTime < 150) {
                appendSignal(0);
            }
            else {
                appendSignal(1);
            }
        } else {
            ++hiTimeTooShort;
        }
    }
	t1 = t2;
}

void dcfInit(int _dcfPin, int _ledPin) {
    dcfPin = _dcfPin;
    ledPin = _ledPin;
    if(_ledPin != -1) {    
        pinMode(ledPin, OUTPUT);
    }
    pinMode(dcfPin, INPUT_PULLUP);
    attachInterrupt(dcfPin, dcf_isr, CHANGE);
    t1 = millis(); 
}

bool dcfDateRequest(struct tm& date) {
    if(validDate) {
        date = dt;
    }
    return validDate;
}

bool dcfDateRequest(unsigned long& loLong, unsigned long& hiLong) {
    if(validDate) {
        loLong = dcfBufferCopy & 0x0000FFFF;
        hiLong = (dcfBufferCopy >> 32) & 0x0000FFFF;
    }
    return validDate;
}

bool dcfStateRequest(std::string& data) {
    static char buffer[65];

    if(messageAvailable) {
        unsigned long long mask = 1;
        sprintf(buffer, "0|00000000|00000|000|000000|0|000000|0|0000000");
        byte pos = strlen(buffer) - 1;

        for(byte i =0; i < bufferPositionCopy; i++) {
            if(i > 20) {
                if((dcfBufferCopy & mask) == mask) {
                    buffer[pos] = '1';
                }
                if(pos == 39 || pos == 37 || pos == 30 || pos == 28 || pos == 21 || pos == 17 || pos == 11 || pos == 2) 
                    pos -= 2;
                else
                    --pos;
            }
            mask <<= 1;
        }
        data += buffer;
        data += "\n";

        if(msg.loTimeTooShort) {
            sprintf(buffer,"Lo-time duration too short:%d ",msg.loTimeTooShort);
            data += buffer;
        }
        if(msg.hiTimeTooShort) {
            sprintf(buffer,"Hi-time duration too short:%d ",msg.hiTimeTooShort);
            data += buffer;
        }
        if(msg.bufferPosition < 59) {
            sprintf(buffer,"Too few impulses :%d ",msg.bufferPosition);
            data += buffer;
        }
        if(msg.bufferPosition > 59) {
            sprintf(buffer, "Too many impulses :%d ",msg.bufferPosition);
            data += buffer;
        }
        if(msg.parityMinute) {
            sprintf(buffer,"Minute parity flag failed ");
            data += buffer;
        }
        if(msg.parityHour) {
            sprintf(buffer, "Hour parity flag failed ");
            data += buffer;
        }
        if(msg.parityDate) {
            sprintf(buffer,"Date parity flag failed ");
            data += buffer;
        }
        data += "\n";
        messageAvailable = false;
    }
    return messageAvailable;
}